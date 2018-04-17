#ifndef NAOFILEREADER_H
#define NAOFILEREADER_H

#include "libnao_global.h"

#include <QFile>

class LIBNAO_API NaoFileReader : public QObject {
    Q_OBJECT

    public:
    NaoFileReader(QString infile);
    NaoFileReader(QIODevice* device, QString filename = QString());

    ~NaoFileReader();

    QString getFileName() const;
    QIODevice* getDevice() const;
    QString fourCC() const;     // first 4 bytes as a string
    qint64 pos() const;

    QByteArray read(qint64 n);
    bool seekRel(qint64 p);         // seek relative to the current position
    bool seek(qint64 p);
    quint8 readUChar();
    qint8 readChar();
    quint16 readUShortLE();
    quint16 readUShortBE();
    static quint16 readUShortLE(char b[2]);
    static quint16 readUShortLE(uchar b[2]);
    static quint16 readUShortBE(char b[2]);
    static quint16 readUShortBE(uchar b[2]);
    qint16 readShortLE();
    qint16 readShortBE();
    static qint16 readShortLE(char b[2]);
    static qint16 readShortLE(uchar b[2]);
    static qint16 readShortBE(char b[2]);
    static qint16 readShortBE(uchar b[2]);
    quint32 readUIntLE();
    quint32 readUIntBE();
    static quint32 readUIntLE(char b[4]);
    static quint32 readUIntLE(uchar b[4]);
    static quint32 readUIntBE(char b[4]);
    static quint32 readUIntBE(uchar b[4]);
    qint32 readIntLE();
    qint32 readIntBE();
    static qint32 readIntLE(char b[4]);
    static qint32 readIntLE(uchar b[4]);
    static qint32 readIntBE(char b[4]);
    static qint32 readIntBE(uchar b[4]);
    quint64 readULongLE();
    quint64 readULongBE();
    static quint64 readULongLE(char b[8]);
    static quint64 readULongLE(uchar b[8]);
    static quint64 readULongBE(char b[8]);
    static quint64 readULongBE(uchar b[8]);
    qint64 readLongLE();
    qint64 readLongBE();
    static qint64 readLongLE(char b[8]);
    static qint64 readLongLE(uchar b[8]);
    static qint64 readLongBE(char b[8]);
    static qint64 readLongBE(uchar b[8]);
    float readFloatLE();
    float readFloatBE();
    static float readFloatLE(char b[4]);
    static float readFloatLE(uchar b[4]);
    static float readFloatBE(char b[4]);
    static float readFloatBE(uchar b[4]);
    double readDoubleLE();
    double readDoubleBE();
    static double readDoubleLE(char b[8]);
    static double readDoubleLE(uchar b[8]);
    static double readDoubleBE(char b[8]);
    static double readDoubleBE(uchar b[8]);
    QString readString();

    protected:
    QString _filename;
    QString _fourCC;

    private:
    QIODevice* _infile;

    void _NaoFileReaderStartup();
};

#endif // NAOFILEREADER_H
