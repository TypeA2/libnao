#include "libnao.h"

#pragma warning(push)
#pragma warning(disable:4100)

#include "vdf_parser.hpp"

#pragma warning(pop)

namespace LibNao {
    namespace Utils {
        QStringList getSupportedExtensions() {
            return QStringList({
                                   "cpk"
                               });
        }

        bool isFileSupported(QString file) {
            for (QString ext : getSupportedExtensions()) {
                if (file.endsWith(ext))
                    return true;
            }

            return false;
        }

        bool isFileSupported(QUrl file) {
            return isFileSupported(file.toString());
        }

        FileType getFileType(QString file) {
            if(isFileSupported(file)) {
                QFile infile(file);
                infile.open(QIODevice::ReadOnly);

                char fourcc[4];
                infile.read(fourcc, 4);
                infile.close();

                if (memcmp(fourcc, "CRID", 4) == 0 || memcmp(fourcc, "CPK ", 4) == 0) {
                    return CRIWare;
                } else if (memcmp(fourcc, "RIFF", 4) == 0 || memcmp(fourcc, "BKHD", 4) == 0) {
                    return WWise;
                } else if (memcmp(fourcc, "DDS ", 4) == 0) {
                    return MS_DDS;
                }
            }

            return None;
        }

        FileType getFileType(QUrl file) {
            return getFileType(file.toString());
        }

        QString getShortSize(quint64 size) {
            if (size > 0x1000000000000000) {
                return QString("%0 EiB").arg((size >> 50) / 1024., 0, 'f', 3);
            } else if (size > 0x4000000000000) {
                return QString("%0 PiB").arg((size >> 40) / 1024., 0, 'f', 3);
            } else if (size > 0x10000000000) {
                return QString("%0 TiB").arg((size >> 30) / 1024., 0, 'f', 3);
            } else if (size > 0x40000000) {
                return QString("%0 GiB").arg((size >> 20) / 1024., 0, 'f', 3);
            } else if (size > 0x100000) {
                return QString("%0 MiB").arg((size >> 10) / 1024., 0, 'f', 3);
            } else if (size > 0x400) {
                return QString("%0 KiB").arg(size / 1024., 0, 'f', 3);
            } else {
                return QString("%0 B").arg(size);
            }
        }
    }

    namespace Steam {
        QString getSteamPath() {
            return QSettings("HKEY_CURRENT_USER\\Software\\Valve\\Steam",
                             QSettings::NativeFormat).value("SteamPath").toString();
        }

        QStringList getSteamInstallFolders() {
            std::ifstream libfolders((getSteamPath() + "/SteamApps/libraryfolders.vdf").toLatin1().constData());
            tyti::vdf::object root = tyti::vdf::read(libfolders);

            QStringList retlist = QStringList(getSteamPath());

            for (size_t i = 1; i <= root.attribs.size() - 2; i++) {
                retlist.append(
                            std::regex_replace(
                                root.attribs.at(std::to_string(i)),
                                std::regex(R"d(\\\\)d"),
                                "/"
                                ).c_str()
                            );
            }

            return retlist;
        }

        QString getGamePath(QString game, QString def) {
            for (QString folder : getSteamInstallFolders()) {
                QStringList entries = QDir(folder + "/SteamApps/common").entryList(QStringList(game), QDir::Dirs);

                if (entries.size() > 0 && QDir(folder + "/SteamApps/common/" + entries.at(0)).count())
                    return folder + "/SteamApps/common/" + entries.at(0);
            }

            return def;
        }
    }
}
