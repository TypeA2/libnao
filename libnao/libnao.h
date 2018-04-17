#ifndef LIBNAO_H
#define LIBNAO_H

#include "libnao_global.h"

#include <fstream>
#include <regex>
#include <memory>
#include <utility>
#include <type_traits>

#include <QDebug>

#include <QSettings>
#include <QDir>
#include <QUrl>

namespace LibNao {
    enum FileType {
        None,
        CRIWare,
        WWise,
        MS_DDS,
        PG_DAT
    };

    namespace Utils {
        // List all file extension we support
        LIBNAO_API QStringList getSupportedExtensions();

        // Shorthand function to check if the file is supported
        LIBNAO_API bool isFileSupported(QString file);
        LIBNAO_API bool isFileSupported(QUrl file);

        // What kind of file is this
        LIBNAO_API FileType getFileType(QString file);
        LIBNAO_API FileType getFileType(QUrl file);

        // Readable filesizes
        LIBNAO_API QString getShortSize(quint64 size, bool bits = false);

        // Readable time
        LIBNAO_API QString getShortTime(double time);

        // Sanitize filenames
        LIBNAO_API QString sanitizeFileName(QString fname);
    }

    namespace Steam {
        // Returns the Steam install folder as read from the Windows registry
        LIBNAO_API QString getSteamPath();

        // Returns a list containing all install folders, including the default
        LIBNAO_API QStringList getSteamInstallFolders();

        // Returns the path of the game folder
        LIBNAO_API QString getGamePath(QString game, QString def = QString());
    }
}

#endif // LIBNAO_H
