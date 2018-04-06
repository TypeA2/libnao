#ifndef NAOCRIWAREREADER_H
#define NAOCRIWAREREADER_H

#include "libnao_global.h"
#include "NaoFileReader.h"

#include <QBuffer>
#include <QVector>
#include <QVariant>
#include <QDebug>

class LIBNAO_API NaoCRIWareReader : public NaoFileReader {
    public:
    NaoCRIWareReader(QString infile);
    NaoCRIWareReader(QIODevice* device);

    struct EmbeddedFile {
        QString origin;
        QString name;
        QString path;
        QString userString;
        qint64 offset;
        qint64 extraOffset;
        qint64 size;
        qint64 extractedSize;
        quint32 id;

        QString localDir;
        quint64 updateDateTime;
    };

    bool isPak() const;
    const QVector<EmbeddedFile>& getFiles() const;

    QByteArray extractFileAt(qint64 index);

    private:
    class UTFReader : public NaoFileReader {
        public:
        UTFReader(QByteArray packet);

        struct UTFRow {
            qint32 type;
            qint64 pos;

            QVariant val;
        };

        struct UTFField {
            char flags;
            quint64 nameOffset;
            QString name;
            QVariant constVal;
        };

        QVector<UTFField>* getFields() const;
        QVector<QVector<UTFRow>*>* getRows() const;
        quint16 getFieldCount() const;
        quint32 getRowCount() const;
        QVariant getFieldData(quint32 row, QString name) const;
        bool hasField(QString name) const;

        private:
        enum StorageFlags : quint32 {
            HasName = 0x10,
            ConstVal = 0x20,
            RowVal = 0x40
        };

        enum TypeFlags : quint32 {
            uChar = 0x00,
            sChar = 0x01,
            uShort = 0x02,
            sShort = 0x03,
            uInt = 0x04,
            sInt = 0x05,
            uLong = 0x06,
            sLong = 0x07,
            sFloat = 0x08,
            sDouble = 0x09,
            String = 0x0A,
            Data = 0x0B
        };

        uchar _encodeType;
        quint16 fieldCount;
        quint32 rowCount;

        QVector<UTFField>* _fields;
        QVector<QVector<UTFRow>*>* _rows;
    };

    bool _isPak;

    qint64 _cpkOffset;
    UTFReader* _cpkUTF = nullptr;

    QVector<EmbeddedFile> files;

    void startup();
    QByteArray readNextUTF();

    QByteArray decompressCRILAYLA(QByteArray file);

    static quint16 getBits(char* input, quint64* offset, uchar* bitpool, quint8* remaining, quint64 bits);
};

#endif // NAOCRIWAREREADER_H
