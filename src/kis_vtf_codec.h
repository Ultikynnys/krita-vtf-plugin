// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QImage>
#include <QIODevice>
#include <QString>

namespace VtfCodec {

struct Header {
    quint32 major = 0;
    quint32 minor = 0;
    quint32 headerSize = 0;
    quint16 width = 0;
    quint16 height = 0;
    quint32 flags = 0;
    quint16 frames = 0;
    qint32 highFormat = -1;
    quint8 mipCount = 0;
    qint32 lowFormat = -1;
    quint8 lowWidth = 0;
    quint8 lowHeight = 0;
};

bool read(QIODevice *device, QImage *image, QString *error);
bool write(QIODevice *device, const QImage &image, QString *error);

} // namespace VtfCodec
