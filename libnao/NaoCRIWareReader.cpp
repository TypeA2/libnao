#include "NaoCRIWareReader.h"

#include <QTextCodec>

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


    _isPak = (_fourCC == "CPK ");

    if(_isPak) {
        seekRel(16);

        _cpkOffset = pos();
        _cpkUTF = new UTFReader(readNextUTF());

        if (_cpkUTF->getFieldData(0, "TocOffset").isValid()) {
            quint64 tocOffset = _cpkUTF->getFieldData(0, "TocOffset").toULongLong();
            quint64 offset = 0;

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

        if (_cpkUTF->getFieldData(0, "EtocOffset").isValid()) {
            seek(_cpkUTF->getFieldData(0, "EtocOffset").toULongLong());

            if (read(4) != QByteArray("ETOC", 4)) {
                qFatal("Invalid ETOC fourCC found");
            }

            seekRel(12);

            UTFReader* filesUTF = new UTFReader(readNextUTF());

            for (int i = 0; i < files.size(); i++) {
                files[i].localDir = filesUTF->getFieldData(i, "LocalDir").toString();
                files[i].updateDateTime = filesUTF->getFieldData(i, "UpdateDateTime").toULongLong();
            }

            delete filesUTF;
        }
    }
}

QByteArray NaoCRIWareReader::readNextUTF() {
    if (QString::fromLatin1(read(4).append('\0')) != "@UTF")
        qFatal("Invalid @UTF fourCC found while reading UTF chunk");

    quint32 packet_size = readUIntBE() + 8;

    seekRel(-8);

    return read(packet_size);
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

    seekRel(1);

    _encodeType = readUChar();
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

        if (field.flags & HasName) {
            field.nameOffset = readUIntBE();

            qint64 pos = this->pos();
            seek(stringsOffset + field.nameOffset);
            field.name = readString();
            seek(pos);
        }

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

            if (storageFlag & ConstVal) {
                row.val = _fields->at(i).constVal;
                rows->append(row);

                continue;
            } else if (storageFlag & RowVal) {
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
    EmbeddedFile file = files.at(index);

    seek(file.extraOffset + file.offset);
    return (file.size == file.extractedSize) ? read(file.size) : decompressCRILAYLA(read(file.size));
}

quint16 NaoCRIWareReader::getBits(char* input, quint64* offset, uchar* bitpool, quint8* remaining, quint64 bits) {
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

