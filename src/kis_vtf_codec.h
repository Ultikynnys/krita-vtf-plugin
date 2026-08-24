// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QImage>
#include <QIODevice>
#include <QString>

namespace VtfCodec {

enum ImageFormat : qint32 {
    RGBA8888 = 0,
    ABGR8888 = 1,
    RGB888 = 2,
    BGR888 = 3,
    RGB565 = 4,
    I8 = 5,
    IA88 = 6,
    A8 = 8,
    ARGB8888 = 11,
    BGRA8888 = 12,
    DXT1 = 13,
    DXT3 = 14,
    DXT5 = 15,
    BGRX8888 = 16,
    DXT1_ONEBITALPHA = 20,
    FORMAT_NONE = -1
};

enum TextureFlag : quint32 {
    PointSample = 0x00000001,
    Trilinear = 0x00000002,
    ClampS = 0x00000004,
    ClampT = 0x00000008,
    Anisotropic = 0x00000010,
    HintDxt5 = 0x00000020,
    PwlCorrected = 0x00000040,
    NormalMap = 0x00000080,
    NoMipmaps = 0x00000100,
    NoLod = 0x00000200,
    AllMips = 0x00000400,
    Procedural = 0x00000800,
    OneBitAlpha = 0x00001000,
    EightBitAlpha = 0x00002000,
    EnvironmentMap = 0x00004000,
    RenderTarget = 0x00008000,
    DepthRenderTarget = 0x00010000,
    NoDebugOverride = 0x00020000,
    SingleCopy = 0x00040000,
    PreSrgb = 0x00080000,
    NoDepthBuffer = 0x00800000,
    ClampU = 0x02000000,
    VertexTexture = 0x04000000,
    SsBump = 0x08000000,
    Border = 0x20000000
};

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

struct WriteOptions {
    quint32 minorVersion = 2;
    ImageFormat imageFormat = RGBA8888;
    quint32 flags = NoMipmaps;
    bool generateMipmaps = false;
    bool generateThumbnail = false;
    quint8 thumbnailSize = 16;
    float bumpScale = 1.0f;
};

QString formatName(ImageFormat format);
bool formatSupportsAlpha(ImageFormat format);
bool validate(const QImage &image, const WriteOptions &options, QString *error);
bool read(QIODevice *device, QImage *image, QString *error);
bool write(QIODevice *device, const QImage &image, const WriteOptions &options, QString *error);
bool write(QIODevice *device, const QImage &image, QString *error);

} // namespace VtfCodec
