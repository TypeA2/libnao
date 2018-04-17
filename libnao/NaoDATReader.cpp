#include "NaoDATReader.h"

#include <windows.h>
#include <QDebug>

NaoDATReader::NaoDATReader(QString infile):
    NaoFileReader(infile),
    fname(infile) {
    startup();
}

NaoDATReader::NaoDATReader(QIODevice *device, QString fname) :
    NaoFileReader(device),
    fname(fname) {
    startup();
}

void NaoDATReader::startup() {
    if (_fourCC != "DAT") {
        qFatal("Invalid DAT fourCC found");
    }

    // skip fourCC

    seekRel(4);

    quint32 fileCount = readUIntLE();
    quint32 filesOffset = readUIntLE();

    // skip extension table offset (extension are included in the name, I'm not sure either why they are also stored seperately)

    seekRel(4);

    quint32 namesOffset = readUIntLE();
    quint32 sizesOffset = readUIntLE();

    seek(namesOffset);

    // names table is prepended with an uint that specifies the alignment of the names
    // names are padded with null, so converting them to a string is easy

    quint32 namesAlignment = readUIntLE();

    files.resize(fileCount);

    // read everything as sequentially as possible (to be nice to the filesystem)
    // first the names

    for (quint32 i = 0; i < fileCount; ++i) {
        files[i].name = QString(read(namesAlignment));
    }

    seek(filesOffset);

    // offsets

    for (quint32 i = 0; i < fileCount; ++i) {
        files[i].offset = readUIntLE();
    }

    seek(sizesOffset);

    // sizes

    for (quint32 i = 0; i < fileCount; ++i) {
        files[i].size = readUIntLE();
    }
}

bool NaoDATReader::extractFileTo(qint64 index, QIODevice *device) {
    // extract to a QIODevice

    if (!device->isWritable()) {
        device->open(QIODevice::WriteOnly);

        if (!device->isWritable()) {
            return false;
        }
    }

    EmbeddedFile file = files.at(index);

    // get the size of the blocks in which we'll be extracting

    SYSTEM_INFO inf;
    GetNativeSystemInfo(&inf);
    const quint32 targetBlockSize = inf.dwPageSize;

    seek(file.offset);

    qint64 remaining = file.size;
    qint64 done = 0;

    emit setExtractMaximum(remaining);

    // read targetBlockSize bytes untill we can read no more

    while (remaining >= targetBlockSize) {
        device->write(read(targetBlockSize));

        remaining-= targetBlockSize;
        done += targetBlockSize;

        if (done % (targetBlockSize * 32) == 0) {
            emit extractProgress(done);
        }
    }

    // read the remaining

    if (remaining > 0) {
        device->write(read(remaining));

        done += remaining;

        emit extractProgress(done);
    }

    return true;
}

const QVector<NaoDATReader::EmbeddedFile>& NaoDATReader::getFiles() const {
    return files;
}

QString NaoDATReader::getFileName() const {
    return fname;
}
