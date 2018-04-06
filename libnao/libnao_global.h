#ifndef LIBNAO_GLOBAL_H
#define LIBNAO_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef LIBNAO_LIBRARY
#define LIBNAO_API Q_DECL_EXPORT
#else
#define LIBNAO_API Q_DECL_IMPORT
#endif

#endif // LIBNAO_GLOBAL_H
