#include "NaoCRIWareReader.h"

#include <QTextCodec>
#include <windows.h>

NaoCRIWareReader::NaoCRIWareReader(QString infile) : NaoFileReader(infile) {
    startup();
}

NaoCRIWareReader::NaoCRIWareReader(QIODevice *device) : NaoFileReader(device) {
    startup();
}

void NaoCRIWareReader::startup() {
    if (_fourCC != "CPK " && _fourCC != "CRID") {
        qFatal("Invalid CRIWare fourCC found");
    }

    // CPK archives start with "CPK ", USM files with CRID
    _isPak = (_fourCC == "CPK ");

    if(_isPak) {
        seekRel(16); // skip fourCC and information on the @UTF that follows (the chunk itself contains the same data)

        _cpkOffset = pos();
        _cpkUTF = new UTFReader(readNextUTF());

        if (_cpkUTF->getFieldData(0, "TocOffset").isValid()) {

            // we have a TOC field

            quint64 tocOffset = _cpkUTF->getFieldData(0, "TocOffset").toULongLong();
            quint64 offset = 0;

            // clamp to 2048
            if (tocOffset > 0x800U) {
                tocOffset = 0x800U;
            }

            if (!_cpkUTF->getFieldData(0, "ContentOffset").isValid()) {
                offset = tocOffset;
            } else if (_cpkUTF->getFieldData(0, "ContentOffset").toULongLong() < tocOffset) {
                offset = _cpkUTF->getFieldData(0, "ContentOffset").toULongLong();
            } else {
                offset = tocOffset;
            }

            seek(_cpkUTF->getFieldData(0, "TocOffset").toULongLong());

            if (read(4) != QByteArray("TOC ", 4)) {
                qFatal("Invalid TOC fourCC found");
            }

            // again skip data on the following @UTF

            seekRel(12);

            UTFReader* filesUTF = new UTFReader(readNextUTF());

            for (quint16 i = 0; i < filesUTF->getRowCount(); i++) {
                EmbeddedFile file;

                file.origin = "TOC ";
                file.name = filesUTF->getFieldData(i, "FileName").toString();
                file.path = filesUTF->getFieldData(i, "DirName").toString();
                file.userString = filesUTF->getFieldData(i, "UserString").toString();
                file.offset = filesUTF->getFieldData(i, "FileOffset").toLongLong();
                file.extraOffset = offset;
                file.size = filesUTF->getFieldData(i, "FileSize").toLongLong();
                file.extractedSize = filesUTF->getFieldData(i, "ExtractSize").toLongLong();
                file.id = filesUTF->getFieldData(i, "ID").toUInt();

                files.push_back(file);
            }

            delete filesUTF;
        }

        // if we have an Etoc (Itoc and Gtoc omitted)

        if (_cpkUTF->getFieldData(0, "EtocOffset").isValid()) {
            seek(_cpkUTF->getFieldData(0, "EtocOffset").toULongLong());

            if (read(4) != QByteArray("ETOC", 4)) {
                qFatal("Invalid ETOC fourCC found");
            }

            // again skip data on the following @UTF

            seekRel(12);

            UTFReader* filesUTF = new UTFReader(readNextUTF());

            // update the possibly cntained values

            for (int i = 0; i < files.size(); i++) {
                files[i].localDir = filesUTF->getFieldData(i, "LocalDir").toString();
                files[i].updateDateTime = filesUTF->getFieldData(i, "UpdateDateTime").toULongLong();
            }

            delete filesUTF;
        }
    } else {
        // skip the foruCC and the following uint blockSize, since we'll be at the end of this block anyway when we finish reading

        seekRel(8);

        quint16 headerSize = readUShortBE();

        if (headerSize != 0x18) {
            qFatal("Expected header size of 0x18");
        }

        qint16 footerSize = readUShortBE();

        if (readUIntBE() != 1) {
            qFatal("CRID block type should be 1");
        }

        // ignore possible sanity check bytes

        seekRel(8);

        if (readUIntBE() != 0 || readUIntBE() != 0) {
            qFatal("Invalid format");
        }

        UTFReader* info = new UTFReader(readNextUTF());

        // we have 1 row for each stream, plus one (the first) for the entire file

        qint8 nStreams = info->getRowCount() - 1;

        // streams are identified by a BE uint that is equal to either '@SFV' or '@SFA' for video and audio respectively

        QMap<quint32, bool> ready;

        for (qint8 i = 1; i <= nStreams; i++) {
            EmbeddedFile file;

            file.name = info->getFieldData(i, "filename").toString();
            file.size = info->getFieldData(i, "filesize").toLongLong();
            file.extractedSize = file.size;
            file.id = info->getFieldData(i, "stmid").toUInt();
            file.type = static_cast<EmbeddedFile::Type>(
                        info->getFieldData(i, "stmid").toUInt() != 0x40534656);
            file.avbps = info->getFieldData(i, "avbps").toLongLong();

            ready.insert(info->getFieldData(i, "stmid").toUInt(), false);

            files.push_back(file);
        }

        delete info;

        // skip footer

        seekRel(footerSize);

        // keep reading as long as not all streams are finished

        while (ready.values().contains(false)) {
            Chunk chunk;
            chunk.offset = pos();

            QByteArray id = read(4); // @SFV or @SFA

            chunk.type = static_cast<Chunk::Type>(id.endsWith('A')); // Booleans map to the enum values
            chunk.size = readUIntBE();
            chunk.headerSize = readUShortBE();
            chunk.footerSize = readUShortBE();
            chunk.dataType = static_cast<Chunk::DataType>(readUIntBE()); // Also maps to the enum values

            // ignore possible sanity check bytes (we're insane)

            seekRel(16);

            if (chunk.dataType == Chunk::StreamInfo) {

                // contains another @UTF with some info on our stream

                UTFReader* info = new UTFReader(readNextUTF());

                EmbeddedFile& file = *std::find_if(files.begin(), files.end(), [&](const EmbeddedFile& f) { return f.id == readUIntBE(id.data()); });

                if (file.type == EmbeddedFile::Video) {
                    file.width = info->getFieldData(0, "width").toLongLong();
                    file.height = info->getFieldData(0, "height").toLongLong();
                    file.totalFrames = info->getFieldData(0, "total_frames").toLongLong();
                    file.nFramerate = info->getFieldData(0, "framerate_n").toLongLong();
                    file.dFramerate = info->getFieldData(0, "framerate_d").toLongLong();
                }

                delete info;
            } else {

                // as long as this chunk does not specify th end of a stream we just skip the contents and save the positions

                if (chunk.size - chunk.headerSize - chunk.footerSize == 0x20) {
                    if (QString::fromLatin1(read(0x20)) == "#CONTENTS END   ===============" || getDevice()->atEnd()) {
                        ready[readUIntBE(id.data())] = true;
                    }
                } else {
                    seekRel(chunk.size - chunk.headerSize - chunk.footerSize);
                }
            }

            seekRel(chunk.footerSize);

            dataChunks.push_back(chunk);
        }
    }
}

QByteArray NaoCRIWareReader::readNextUTF() {
    if (read(4) != QByteArray("@UTF", 4))
        qFatal("Invalid @UTF fourCC found while reading UTF chunk");

    quint32 packetSize = readUIntBE() + 8; // first uint is the remaining packet size

    seekRel(-8);

    return read(packetSize); // read everything including the fourCC
}

bool NaoCRIWareReader::isPak() const {
    return _isPak;
}

const QVector<NaoCRIWareReader::EmbeddedFile>& NaoCRIWareReader::getFiles() const {
    return files;
}

NaoCRIWareReader::UTFReader::UTFReader(QByteArray packet) :
    NaoFileReader(new QBuffer(&packet)),
    _fields(new QVector<UTFField>()),
    _rows(new QVector<QVector<UTFRow>*>()) {
    if (read(4) != QByteArray("@UTF", 4)) {
        qFatal("Invalid @UTF fourCC found");
    }

    // Read the UTF header

    qint32 tableSize = readUIntBE();

    seekRel(1); // unused byte

    _encodeType = readUChar(); // encoding: Shift-JIS if 0, else UTF-8
    quint16 rowsOffset = readUShortBE() + 8;
    quint32 stringsOffset = readUIntBE() + 8;
    quint32 dataOffset = readUIntBE() + 8;
    quint32 tableNameOffset = readUIntBE() + 8;
    fieldCount = readUShortBE();
    quint16 rowSize = readUShortBE();
    rowCount = readUIntBE();

    Q_UNUSED(tableSize);
    Q_UNUSED(tableNameOffset);
    Q_UNUSED(rowSize);

    // Then read fieldCount fields, consisting of 1 byte flags,
    // then (if flags & HasName is nonzero) an uint pointing to the field name

    for (quint16 i = 0; i < fieldCount; i++) {
        UTFField field;
        field.flags = readChar();

        // sanity check for name (wait we were insane weren't we?!)

        if (field.flags & HasName) {
            field.nameOffset = readUIntBE();

            qint64 pos = this->pos();
            seek(stringsOffset + field.nameOffset);
            field.name = readString();
            seek(pos);
        }

        // const values if applicable

        if (field.flags & ConstVal) {
            switch (field.flags & 0x0F) {
                case uChar:
                    field.constVal = QVariant::fromValue(readUChar());
                    break;

                case sChar:
                    field.constVal = QVariant::fromValue(readChar());
                    break;

                case uShort:
                    field.constVal = QVariant::fromValue(readUShortBE());
                    break;

                case sShort:
                    field.constVal = QVariant::fromValue(readShortBE());
                    break;

                case uInt:
                    field.constVal = QVariant::fromValue(readUIntBE());
                    break;

                case sInt:
                    field.constVal = QVariant::fromValue(readIntBE());
                    break;

                case uLong:
                    field.constVal = QVariant::fromValue(readULongBE());
                    break;

                case sLong:
                    field.constVal = QVariant::fromValue(readLongBE());
                    break;

                case sFloat:
                    field.constVal = QVariant::fromValue(readFloatBE());
                    break;

                case sDouble:
                    field.constVal = QVariant::fromValue(readDoubleBE());
                    break;

                case String: {
                    quint32 offset = readUIntBE();
                    qint64 pos = this->pos();
                    seek(stringsOffset + offset);

                    // read in appropiate encoding. Shift-JIS is still null-terminated, only the byte format is weird.

                    field.constVal = QVariant::fromValue(
                                QTextCodec::codecForName(
                                    (_encodeType == 0) ? "Shift-JIS" : "UTF-8")->toUnicode(
                                        readString().toLatin1()));
                    seek(pos);

                    break;
                }

                case Data: {
                    quint32 offset = readUIntBE();
                    quint32 size = readUIntBE();
                    qint64 pos = this->pos();
                    seek(dataOffset + offset);
                    field.constVal = QVariant::fromValue(read(size));
                    seek(pos);

                    break;
                }
            }
        }

        _fields->append(field);
    }

    seek(rowsOffset);

    for (quint32 j = 0; j < rowCount; j++) {
        QVector<UTFRow>* rows = new QVector<UTFRow>();

        for (quint16 i = 0; i < fieldCount; i++) {
            UTFRow row;

            quint32 storageFlag = _fields->at(i).flags & 0xF0;

            // skip if we already have a constant value

            if (storageFlag & ConstVal) {
                row.val = _fields->at(i).constVal;
                rows->append(row);

                continue;
            } else if (storageFlag & RowVal) {

                // more of a sanity check (oops)

                row.type = _fields->at(i).flags & 0x0F;
                row.pos = pos();

                switch (row.type) {
                    case uChar:
                        row.val = QVariant::fromValue(readUChar());
                        break;

                    case sChar:
                        row.val = QVariant::fromValue(readChar());
                        break;

                    case uShort:
                        row.val = QVariant::fromValue(readUShortBE());
                        break;

                    case sShort:
                        row.val = QVariant::fromValue(readShortBE());
                        break;

                    case uInt:
                        row.val = QVariant::fromValue(readUIntBE());
                        break;

                    case sInt:
                        row.val = QVariant::fromValue(readIntBE());
                        break;

                    case uLong:
                        row.val = QVariant::fromValue(readULongBE());
                        break;

                    case sLong:
                        row.val = QVariant::fromValue(readLongBE());
                        break;

                    case sFloat:
                        row.val = QVariant::fromValue(readFloatBE());
                        break;

                    case sDouble:
                        row.val = QVariant::fromValue(readDoubleBE());
                        break;

                    case String: {
                        quint32 offset = readUIntBE();
                        qint64 pos = this->pos();

                        seek(stringsOffset + offset);
                        row.val = QVariant::fromValue(
                                    QTextCodec::codecForName(
                                        (_encodeType == 0) ? "Shift-JIS" : "UTF-8")->toUnicode(
                                            readString().toLatin1()));
                        seek(pos);

                        break;
                    }

                    case Data: {
                        quint32 offset = readUIntBE();
                        quint32 size = readUIntBE();
                        qint64 pos = this->pos();
                        seek(dataOffset + offset);
                        row.val = QVariant::fromValue(read(size));
                        seek(pos);

                        break;
                    }
                }
            } else {
                row.val = QVariant();
            }

            rows->append(row);
        }

        _rows->append(rows);
    }
}

QVector<NaoCRIWareReader::UTFReader::UTFField>* NaoCRIWareReader::UTFReader::getFields() const {
    return _fields;
}

QVector<QVector<NaoCRIWareReader::UTFReader::UTFRow>*>* NaoCRIWareReader::UTFReader::getRows() const {
    return _rows;
}

quint16 NaoCRIWareReader::UTFReader::getFieldCount() const {
    return fieldCount;
}

quint32 NaoCRIWareReader::UTFReader::getRowCount() const {
    return rowCount;
}

QVariant NaoCRIWareReader::UTFReader::getFieldData(quint32 row, QString name) const {

    // we could use std::find_if, but then  again, we could also not (number of files shouldn't be too crazy anyway)

    for (quint16 i = 0; i < fieldCount; i++) {
        if (_fields->at(i).name == name) {
            return _rows->at(row)->at(i).val;
        }
    }

    return QVariant();
}

bool NaoCRIWareReader::UTFReader::hasField(QString name) const {
    for (quint16 i = 0; i < fieldCount; i++) {
        if (_fields->at(i).name == name) {
            return true;
        }
    }

    return false;
}

QByteArray NaoCRIWareReader::extractFileAt(qint64 index) {

    // in-memory extraction

    EmbeddedFile file = files.at(index);

    if (_isPak) {
        seek(file.extraOffset + file.offset);
        return (file.size == file.extractedSize) ? read(file.size) : decompressCRILAYLA(read(file.size));
    } else {
        QVector<Chunk> chunks;

        // gather all data chunks

        for (QVector<Chunk>::iterator it = dataChunks.begin();
             it != dataChunks.end(); ++it) {
            Chunk chunk = *it;

            if (chunk.type == static_cast<Chunk::Type>(file.type) && chunk.dataType == Chunk::Data) {
                chunks.push_back(chunk);
            }
        }

        QByteArray output;

        // read all data those chunks point to

        for (QVector<Chunk>::iterator it = chunks.begin();
             it != chunks.end(); ++it) {
            Chunk chunk = *it;

            seek(chunk.offset + chunk.headerSize);

            output.append(read(chunk.size - chunk.headerSize - chunk.footerSize));
        }

        return output;
    }
}

bool NaoCRIWareReader::extractFileTo(qint64 index, QIODevice* device) {

    // extract to a QIODevice in chunks equal to the page size of the filesystem (if the QIODevice is in memory, well tough)

    if (!device->isWritable()) {
        device->open(QIODevice::WriteOnly);

        if (!device->isWritable()) {
            return false;
        }
    }

    EmbeddedFile file = files.at(index);

    // get the page size from Windows

    SYSTEM_INFO inf;
    GetNativeSystemInfo(&inf);
    const quint32 targetBlockSize = inf.dwPageSize;

    if (_isPak) {
        seek(file.extraOffset + file.offset);

        // if the file is compressed we have no choice but to still completely load it into memory. This is fine because compressed files usually have limited size.

        if (file.size == file.extractedSize) {
            qint64 remaining = file.size;
            qint64 hold = 0;

            // read targetBlockSize bytes as long as we can

            while (remaining >= targetBlockSize) {
                device->write(read(targetBlockSize));

                remaining -= targetBlockSize;
                hold += targetBlockSize;

                // prevents spamming signals/slots

                if (hold % (targetBlockSize * 32) == 0) {
                    emit extractProgress(file.size - remaining, file.size);
                }
            }

            // read remaining bytes

            if (remaining > 0) {
                device->write(read(remaining));

                remaining = 0;

                emit extractProgress(file.size, file.size);
            }
        } else {
            device->write(decompressCRILAYLA(read(file.size)));
        }

        return true;
    } else {
        QVector<Chunk> chunks;

        qint64 totalSize = 0;

        // gather data chunks

        for (QVector<Chunk>::iterator it = dataChunks.begin();
             it != dataChunks.end(); ++it) {
            Chunk chunk = *it;

            if (chunk.type == static_cast<Chunk::Type>(file.type) && chunk.dataType == Chunk::Data) {
                chunks.push_back(chunk);

                totalSize += (chunk.size - chunk.headerSize - chunk.footerSize);
            }
        }

        qint64 done = 0;

        // read data chunks in blocks agains

        for (QVector<Chunk>::iterator it = chunks.begin();
             it != chunks.end(); ++it) {
            Chunk chunk = *it;

            seek(chunk.offset + chunk.headerSize);

            qint64 remaining = chunk.size - chunk.headerSize - chunk.footerSize;

            while (remaining >= targetBlockSize) {
                device->write(read(targetBlockSize));

                remaining -= targetBlockSize;
                done += targetBlockSize;

                if (done % (targetBlockSize * 32) == 0) {
                    emit extractProgress(done, totalSize);
                }
            }

            if (remaining > 0) {
                device->write(read(remaining));

                done += remaining;

                emit extractProgress(done, totalSize);
            }
        }

        return true;
    }
}

quint16 NaoCRIWareReader::getBits(char* input, quint64* offset, uchar* bitpool, quint8* remaining, quint64 bits) {

    // WARNING: DIRTY C CODE IN C++
    // Reads bits from input, storing bits in bitpool (etc)

    quint16 output = 0;
    quint64 outbits = 0;
    quint64 bitsNow = 0;

    while (outbits < bits) {
        if (*remaining == 0) {
            *bitpool = input[*offset];
            *remaining = 8;
            --*offset;
        }

        if (*remaining > (bits - outbits)) {
            bitsNow = bits - outbits;
        } else {
            bitsNow = *remaining;
        }

        output <<= bitsNow;
        output |= static_cast<quint16>(
                    static_cast<quint16>(
                        *bitpool >> (*remaining - bitsNow)) & ((1 << bitsNow) - 1));

        *remaining -= bitsNow;
        outbits += bitsNow;
    }

    return output;
}

QByteArray NaoCRIWareReader::decompressCRILAYLA(QByteArray file) {
    // ALSO WARNING: ALSO (albeit less so) DIRTY C CODE IN CPP
    // decompress CRILAYLA (couldn't they just use gzip or something?)

    quint64 size = file.size();
    char* data = file.data();

    if (file.mid(0, 8) != QByteArray("CRILAYLA", 8)) {
        qFatal("Invalid CRILAYLA signature found");
    }

    quint32 expectedSize = readUIntLE(&data[8]);
    quint32 headerOffset = readUIntLE(&data[12]);

    QByteArray result(expectedSize + 0x100, '\0');
    char* outdata = result.data();

    memcpy_s(outdata, 0x100, &data[headerOffset + 0x10], 0x100);

    quint64 inputEnd = size - 0x101;
    quint64 inputOffset = inputEnd;
    quint64 outputEnd = 0xFF + expectedSize;
    uchar bitpool = 0;
    quint8 remaining = 0;
    quint64 outputSize = 0;
    constexpr quint32 levels[4] = { 2, 3, 5, 8 };

    while (outputSize < expectedSize) {
        if (getBits(data, &inputOffset, &bitpool, &remaining, 1) > 0) {
            quint64 backreferenceOffset = outputEnd - outputSize + getBits(data, &inputOffset, &bitpool, &remaining, 13) + 3;
            quint64 backreferenceLength = 3;
            quint32 level;

            for (level = 0; level < 4; level++) {
                quint16 thisLevel = getBits(data, &inputOffset, &bitpool, &remaining, levels[level]);
                backreferenceLength += thisLevel;

                if (thisLevel != ((1 << levels[level]) - 1)) {
                    break;
                }
            }

            if (level == 4) {
                quint16 thisLevel;

                do {
                    thisLevel = getBits(data, &inputOffset, &bitpool, &remaining, 8);
                    backreferenceLength += thisLevel;
                } while (thisLevel == 0xFF);
            }

            for (quint64 i = 0; i < backreferenceLength; i++) {
                outdata[outputEnd - outputSize] = outdata[backreferenceOffset--];
                outputSize++;
            }
        } else {
            outdata[outputEnd - outputSize] = static_cast<char>(getBits(data, &inputOffset, &bitpool, &remaining, 8));
            outputSize++;
        }
    }

    return result;
}

