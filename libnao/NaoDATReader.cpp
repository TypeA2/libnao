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

    seekRel(4);

    quint32 fileCount = readUIntLE();
    quint32 filesOffset = readUIntLE();

    seekRel(4);

    quint32 namesOffset = readUIntLE();
    quint32 sizesOffset = readUIntLE();

    seek(namesOffset);

    quint32 namesAlignment = readUIntLE();

    files.resize(fileCount);

    for (quint32 i = 0; i < fileCount; ++i) {
        files[i].name = QString(read(namesAlignment));
    }

    seek(filesOffset);

    for (quint32 i = 0; i < fileCount; ++i) {
        files[i].offset = readUIntLE();
    }

    seek(sizesOffset);

    for (quint32 i = 0; i < fileCount; ++i) {
        files[i].size = readUIntLE();
    }
}

bool NaoDATReader::extractFileTo(qint64 index, QIODevice *device) {
    if (!device->isWritable()) {
        device->open(QIODevice::WriteOnly);

        if (!device->isWritable()) {
            return false;
        }
    }

    EmbeddedFile file = files.at(index);

    SYSTEM_INFO inf;
    GetNativeSystemInfo(&inf);
    const quint32 targetBlockSize = inf.dwPageSize;

    seek(file.offset);

    qint64 remaining = file.size;
    qint64 done = 0;

    emit setExtractMaximum(remaining);

    while (remaining >= targetBlockSize) {
        device->write(read(targetBlockSize));

        remaining-= targetBlockSize;
        done += targetBlockSize;

        if (done % (targetBlockSize * 32) == 0) {
            emit extractProgress(done);
        }
    }

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
