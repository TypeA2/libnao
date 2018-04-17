#include "NaoFileReader.h"

// I quoteth "REINTERPRET_CAST SHOULD BE USED WITH CAUTION"

// it's safe to assume this code runs on little-endian systems,
// so we can just reinterpret_cast char pointers as integer pointers of the appropiate type for LE conversions

NaoFileReader::NaoFileReader(QString infile) :
    _filename(infile),
    _infile(new QFile(infile)) {
    _infile->open(QIODevice::ReadOnly);

    _NaoFileReaderStartup();
}

NaoFileReader::NaoFileReader(QIODevice *device, QString filename) :
    _infile(device),
    _filename(filename) {
    if (!_infile->isReadable())
        _infile->open(QIODevice::ReadOnly);

    _NaoFileReaderStartup();
}

void NaoFileReader::_NaoFileReaderStartup() {
    // read + store fourCC, then seek back to the start

    _fourCC = QString::fromLatin1(_infile->read(4).append('\0'));
    seekRel(-4);
}

QString NaoFileReader::fourCC() const {
    return _fourCC;
}

qint64 NaoFileReader::pos() const {
    return _infile->pos();
}

QByteArray NaoFileReader::read(qint64 n) {
    return _infile->read(n);
}

bool NaoFileReader::seekRel(qint64 p) {
    return _infile->seek(_infile->pos() + p);
}

bool NaoFileReader::seek(qint64 p) {
    return _infile->seek(p);
}

quint8 NaoFileReader::readUChar() {
    return *reinterpret_cast<uchar*>(read(1).data());
}

qint8 NaoFileReader::readChar() {
    return *read(1);
}

quint16 NaoFileReader::readUShortLE() {
    return *reinterpret_cast<quint16*>(read(2).data());
}

quint16 NaoFileReader::readUShortBE() {
    QByteArray r = read(2);
    quint16 v = 0;

    for (int i = 0; i < 2; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF); // Sometimes the upper bits get a nonzero value, so we extract the lower bits like this
    }

    return v;
}

quint16 NaoFileReader::readUShortLE(char b[2]) {
    return *reinterpret_cast<quint16*>(b);
}

quint16 NaoFileReader::readUShortLE(uchar b[2]) {
    return *reinterpret_cast<quint16*>(b);
}

quint16 NaoFileReader::readUShortBE(char b[2]) {
    return readUShortBE(reinterpret_cast<uchar*>(b));
}

quint16 NaoFileReader::readUShortBE(uchar b[2]) {
    quint16 v = 0;

    for (int i = 0; i < 2; i++) {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

qint16 NaoFileReader::readShortLE() {
    return *reinterpret_cast<qint16*>(read(2).data());
}

qint16 NaoFileReader::readShortBE() {
    QByteArray r = read(2);
    qint16 v = 0;

    for (int i = 0; i < 2; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return v;
}

qint16 NaoFileReader::readShortLE(char b[2]) {
    return *reinterpret_cast<qint16*>(b);
}

qint16 NaoFileReader::readShortLE(uchar b[2]) {
    return *reinterpret_cast<qint16*>(b);
}

qint16 NaoFileReader::readShortBE(char b[2]) {
    return readShortBE(reinterpret_cast<uchar*>(b));
}

qint16 NaoFileReader::readShortBE(uchar b[2]) {
    qint16 v = 0;

    for (int i = 0; i < 2; i++) {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

quint32 NaoFileReader::readUIntLE() {
    return *reinterpret_cast<quint32*>(read(4).data());
}

quint32 NaoFileReader::readUIntBE() {
    QByteArray r = read(4);
    quint32 v = 0;

    for (int i = 0; i < 4; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return v;
}

quint32 NaoFileReader::readUIntLE(char b[4]) {
    return *reinterpret_cast<quint32*>(b);
}

quint32 NaoFileReader::readUIntLE(uchar b[4]) {
    return *reinterpret_cast<quint32*>(b);
}

quint32 NaoFileReader::readUIntBE(char b[4]) {
    return readUIntBE(reinterpret_cast<uchar*>(b));
}

quint32 NaoFileReader::readUIntBE(uchar b[4]) {
    quint32 v = 0;

    for (int i = 0; i < 4; i++) {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

qint32 NaoFileReader::readIntLE() {
    return *reinterpret_cast<qint32*>(read(4).data());
}

qint32 NaoFileReader::readIntBE() {
    QByteArray r = read(4);
    qint32 v = 0;

    for (int i = 0; i < 4; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return v;
}

qint32 NaoFileReader::readIntLE(char b[4]) {
    return *reinterpret_cast<qint32*>(b);
}

qint32 NaoFileReader::readIntLE(uchar b[4]) {
    return *reinterpret_cast<qint32*>(b);
}

qint32 NaoFileReader::readIntBE(char b[4]) {
    return readIntBE(reinterpret_cast<uchar*>(b));
}

qint32 NaoFileReader::readIntBE(uchar b[4]) {
    qint32 v = 0;

    for (int i = 0; i < 4; i++) {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

quint64 NaoFileReader::readULongLE() {
    return *reinterpret_cast<quint64*>(read(8).data());
}

quint64 NaoFileReader::readULongBE() {
    QByteArray r = read(8);
    quint64 v = 0;

    for (int i = 0; i < 8; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return v;
}

quint64 NaoFileReader::readULongLE(char b[8]) {
    return *reinterpret_cast<quint64*>(b);
}

quint64 NaoFileReader::readULongLE(uchar b[8]) {
    return *reinterpret_cast<quint64*>(b);
}

quint64 NaoFileReader::readULongBE(char b[8]) {
    return readULongBE(reinterpret_cast<uchar*>(b));
}

quint64 NaoFileReader::readULongBE(uchar b[8]) {
    quint64 v = 0;

    for (int i = 0; i < 8; i++) {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

qint64 NaoFileReader::readLongLE() {
    return *reinterpret_cast<qint64*>(read(8).data());
}

qint64 NaoFileReader::readLongBE() {
    QByteArray r = read(8);
    qint64 v = 0;

    for (int i = 0; i < 8; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return v;
}

qint64 NaoFileReader::readLongLE(char b[8]) {
    return *reinterpret_cast<qint64*>(b);
}

qint64 NaoFileReader::readLongLE(uchar b[8]) {
    return *reinterpret_cast<qint64*>(b);
}

qint64 NaoFileReader::readLongBE(char b[8]) {
    return readLongBE(reinterpret_cast<uchar*>(b));
}

qint64 NaoFileReader::readLongBE(uchar b[8]) {
    qint64 v = 0;

    for (int i = 0; i < 8; i++) {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

float NaoFileReader::readFloatLE() {
    return *reinterpret_cast<float*>(read(4).data());
}

float NaoFileReader::readFloatBE() {
    QByteArray r = read(4);
    quint32 v = 0;

    for (int i = 0; i < 4; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return *reinterpret_cast<float*>(&v);
}

float NaoFileReader::readFloatLE(char b[4]) {
    return *reinterpret_cast<float*>(b);
}

float NaoFileReader::readFloatLE(uchar b[4]) {
    return *reinterpret_cast<float*>(b);
}

float NaoFileReader::readFloatBE(char b[4]) {
    return readFloatBE(reinterpret_cast<uchar*>(b));
}

float NaoFileReader::readFloatBE(uchar b[4]) {
    quint32 v = 0;

    for (int i = 0; i < 4; i++) {
        v <<= 8;
        v |= b[i];
    }

    return *reinterpret_cast<float*>(&v);
}

double NaoFileReader::readDoubleLE() {
    return *reinterpret_cast<double*>(read(8).data());
}

double NaoFileReader::readDoubleBE() {
    QByteArray r = read(8);
    quint64 v = 0;

    for (int i = 0; i < 8; i++) {
        v <<= 8;
        v |= (r.at(i) & 0xFF);
    }

    return *reinterpret_cast<double*>(&v);
}

double NaoFileReader::readDoubleLE(char b[8]) {
    return *reinterpret_cast<double*>(b);
}

double NaoFileReader::readDoubleLE(uchar b[8]) {
    return *reinterpret_cast<float*>(b);
}

double NaoFileReader::readDoubleBE(char b[8]) {
    return readDoubleBE(reinterpret_cast<uchar*>(b));
}

double NaoFileReader::readDoubleBE(uchar b[8]) {
    quint64 v = 0;

    for (int i = 0; i < 8; i++) {
        v <<= 8;
        v |= b[i];
    }

    return *reinterpret_cast<double*>(&v);
}

QString NaoFileReader::readString() {
    QByteArray r;

    // read untill we find a null

    do {
        r.append(*read(1));
    } while (r.at(r.size() - 1) != '\0');

    return QString::fromLatin1(r);
}

NaoFileReader::~NaoFileReader() {
    _infile->close();
}

QString NaoFileReader::getFileName() const {
    return _filename;
}

QIODevice* NaoFileReader::getDevice() const {
    return _infile;
}
