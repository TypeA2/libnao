#ifndef NAODATREADER_H
#define NAODATREADER_H

#include "libnao_global.h"
#include "NaoFileReader.h"

#include <QVector>

class LIBNAO_API NaoDATReader : public NaoFileReader {
    Q_OBJECT

    public:
    NaoDATReader(QString infile);
    NaoDATReader(QIODevice* device, QString fname = QString());

    struct EmbeddedFile {
        QString name;
        quint32 offset;
        quint32 size;
    };

    const QVector<EmbeddedFile>& getFiles() const;
    QString getFileName() const;

    bool extractFileTo(qint64 index, QIODevice* device);

    signals:
    void extractProgress(const qint64 current);
    void setExtractMaximum(const qint64 max);

    private:
    void startup();

    QString fname;
    QVector<EmbeddedFile> files;
};

#endif // NAODATREADER_H
