// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_codec.h"

#include <QDataStream>
#include <QtEndian>

namespace {

enum ImageFormat : qint32 {
    RGBA8888 = 0, ABGR8888 = 1, RGB888 = 2, BGR888 = 3, RGB565 = 4,
    I8 = 5, IA88 = 6, A8 = 8, ARGB8888 = 11, BGRA8888 = 12,
    DXT1 = 13, DXT3 = 14, DXT5 = 15, BGRX8888 = 16,
    DXT1_ONEBITALPHA = 20, FORMAT_NONE = -1
};

quint32 readU32(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + offset));
}

qint32 readI32(const QByteArray &data, int offset)
{
    return qint32(readU32(data, offset));
}

quint16 readU16(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(data.constData() + offset));
}

int imageSize(int width, int height, qint32 format)
{
    if (format == DXT1 || format == DXT1_ONEBITALPHA) {
        return qMax(1, (width + 3) / 4) * qMax(1, (height + 3) / 4) * 8;
    }
    if (format == DXT3 || format == DXT5) {
        return qMax(1, (width + 3) / 4) * qMax(1, (height + 3) / 4) * 16;
    }
    switch (format) {
    case RGBA8888: case ABGR8888: case ARGB8888: case BGRA8888: case BGRX8888:
        return width * height * 4;
    case RGB888: case BGR888:
        return width * height * 3;
    case RGB565: case IA88:
        return width * height * 2;
    case I8: case A8:
        return width * height;
    default:
        return -1;
    }
}

QRgb rgb565(quint16 value)
{
    const int r5 = (value >> 11) & 31;
    const int g6 = (value >> 5) & 63;
    const int b5 = value & 31;
    return qRgba((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4),
                 (b5 << 3) | (b5 >> 2), 255);
}

QRgb interpolate(QRgb a, QRgb b, int wa, int wb, int divisor, int alpha = 255)
{
    return qRgba((qRed(a) * wa + qRed(b) * wb) / divisor,
                 (qGreen(a) * wa + qGreen(b) * wb) / divisor,
                 (qBlue(a) * wa + qBlue(b) * wb) / divisor, alpha);
}

void decodeColors(const uchar *block, QRgb palette[4], quint32 *indices, bool forceFour)
{
    const quint16 c0 = qFromLittleEndian<quint16>(block);
    const quint16 c1 = qFromLittleEndian<quint16>(block + 2);
    palette[0] = rgb565(c0);
    palette[1] = rgb565(c1);
    if (forceFour || c0 > c1) {
        palette[2] = interpolate(palette[0], palette[1], 2, 1, 3);
        palette[3] = interpolate(palette[0], palette[1], 1, 2, 3);
    } else {
        palette[2] = interpolate(palette[0], palette[1], 1, 1, 2);
        palette[3] = qRgba(0, 0, 0, 0);
    }
    *indices = qFromLittleEndian<quint32>(block + 4);
}

void decodeBlock(const uchar *block, qint32 format, QRgb pixels[16])
{
    QRgb palette[4];
    quint32 colorIndices = 0;
    const uchar *colorBlock = format == DXT1 || format == DXT1_ONEBITALPHA ? block : block + 8;
    decodeColors(colorBlock, palette, &colorIndices, format != DXT1 && format != DXT1_ONEBITALPHA);

    if (format == DXT1 || format == DXT1_ONEBITALPHA) {
        for (int i = 0; i < 16; ++i) pixels[i] = palette[(colorIndices >> (2 * i)) & 3];
        return;
    }
    if (format == DXT3) {
        const quint64 alphaBits = qFromLittleEndian<quint64>(block);
        for (int i = 0; i < 16; ++i) {
            const QRgb c = palette[(colorIndices >> (2 * i)) & 3];
            pixels[i] = qRgba(qRed(c), qGreen(c), qBlue(c), ((alphaBits >> (4 * i)) & 15) * 17);
        }
        return;
    }

    const int a0 = block[0], a1 = block[1];
    int alpha[8] = {a0, a1, 0, 0, 0, 0, 0, 0};
    if (a0 > a1) {
        for (int i = 0; i < 6; ++i) alpha[i + 2] = (a0 * (6 - i) + a1 * (i + 1)) / 7;
    } else {
        for (int i = 0; i < 4; ++i) alpha[i + 2] = (a0 * (4 - i) + a1 * (i + 1)) / 5;
        alpha[6] = 0; alpha[7] = 255;
    }
    quint64 alphaBits = 0;
    for (int i = 0; i < 6; ++i) alphaBits |= quint64(block[i + 2]) << (8 * i);
    for (int i = 0; i < 16; ++i) {
        const QRgb c = palette[(colorIndices >> (2 * i)) & 3];
        pixels[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha[(alphaBits >> (3 * i)) & 7]);
    }
}

bool decode(const QByteArray &raw, int width, int height, qint32 format, QImage *image, QString *error)
{
    *image = QImage(width, height, QImage::Format_RGBA8888);
    if (image->isNull()) { *error = QStringLiteral("Could not allocate the VTF image"); return false; }

    if (format == DXT1 || format == DXT1_ONEBITALPHA || format == DXT3 || format == DXT5) {
        const int blockSize = format == DXT1 || format == DXT1_ONEBITALPHA ? 8 : 16;
        int offset = 0;
        for (int by = 0; by < (height + 3) / 4; ++by) {
            for (int bx = 0; bx < (width + 3) / 4; ++bx) {
                QRgb pixels[16];
                decodeBlock(reinterpret_cast<const uchar *>(raw.constData() + offset), format, pixels);
                offset += blockSize;
                for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                    const int x = bx * 4 + px, y = by * 4 + py;
                    if (x < width && y < height) image->setPixel(x, y, pixels[py * 4 + px]);
                }
            }
        }
        return true;
    }

    int offset = 0;
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        QRgb pixel;
        const uchar *p = reinterpret_cast<const uchar *>(raw.constData() + offset);
        switch (format) {
        case RGBA8888: pixel = qRgba(p[0], p[1], p[2], p[3]); offset += 4; break;
        case BGRA8888: pixel = qRgba(p[2], p[1], p[0], p[3]); offset += 4; break;
        case ABGR8888: pixel = qRgba(p[3], p[2], p[1], p[0]); offset += 4; break;
        case ARGB8888: pixel = qRgba(p[1], p[2], p[3], p[0]); offset += 4; break;
        case BGRX8888: pixel = qRgba(p[2], p[1], p[0], 255); offset += 4; break;
        case RGB888: pixel = qRgba(p[0], p[1], p[2], 255); offset += 3; break;
        case BGR888: pixel = qRgba(p[2], p[1], p[0], 255); offset += 3; break;
        case RGB565: pixel = rgb565(qFromLittleEndian<quint16>(p)); offset += 2; break;
        case I8: pixel = qRgba(p[0], p[0], p[0], 255); offset += 1; break;
        case IA88: pixel = qRgba(p[0], p[0], p[0], p[1]); offset += 2; break;
        case A8: pixel = qRgba(255, 255, 255, p[0]); offset += 1; break;
        default: *error = QStringLiteral("Unsupported VTF pixel format %1").arg(format); return false;
        }
        image->setPixel(x, y, pixel);
    }
    return true;
}

} // namespace

namespace VtfCodec {

bool read(QIODevice *device, QImage *image, QString *error)
{
    if (!device || !device->isReadable()) { *error = QStringLiteral("The VTF stream is not readable"); return false; }
    const QByteArray data = device->readAll();
    if (data.size() < 64 || data.left(4) != QByteArray("VTF\0", 4)) {
        *error = QStringLiteral("Invalid VTF signature or truncated header"); return false;
    }
    Header h;
    h.major = readU32(data, 4); h.minor = readU32(data, 8); h.headerSize = readU32(data, 12);
    h.width = readU16(data, 16); h.height = readU16(data, 18); h.flags = readU32(data, 20);
    h.frames = readU16(data, 24); h.highFormat = readI32(data, 52); h.mipCount = quint8(data[56]);
    h.lowFormat = readI32(data, 57); h.lowWidth = quint8(data[61]); h.lowHeight = quint8(data[62]);
    if (h.major != 7 || h.headerSize < 64 || h.headerSize > quint32(data.size()) || !h.width || !h.height || !h.mipCount) {
        *error = QStringLiteral("Unsupported or invalid VTF header"); return false;
    }
    int offset = int(h.headerSize);
    if (h.lowFormat != FORMAT_NONE && h.lowWidth && h.lowHeight) {
        const int lowSize = imageSize(h.lowWidth, h.lowHeight, h.lowFormat);
        if (lowSize < 0) { *error = QStringLiteral("Unsupported VTF thumbnail format"); return false; }
        offset += lowSize;
    }
    for (int level = h.mipCount - 1; level > 0; --level) {
        const int size = imageSize(qMax(1, int(h.width) >> level), qMax(1, int(h.height) >> level), h.highFormat);
        if (size < 0) { *error = QStringLiteral("Unsupported VTF pixel format %1").arg(h.highFormat); return false; }
        offset += size;
    }
    const int size = imageSize(h.width, h.height, h.highFormat);
    if (size < 0 || offset < 0 || offset + size > data.size()) {
        *error = size < 0 ? QStringLiteral("Unsupported VTF pixel format %1").arg(h.highFormat)
                         : QStringLiteral("The VTF image data is truncated");
        return false;
    }
    return decode(data.mid(offset, size), h.width, h.height, h.highFormat, image, error);
}

bool write(QIODevice *device, const QImage &source, QString *error)
{
    if (!device || !device->isWritable()) { *error = QStringLiteral("The VTF stream is not writable"); return false; }
    if (source.isNull() || source.width() > 65535 || source.height() > 65535) {
        *error = QStringLiteral("Invalid image dimensions for VTF"); return false;
    }
    const QImage image = source.convertToFormat(QImage::Format_RGBA8888);
    QByteArray header(80, '\0');
    header.replace(0, 4, QByteArray("VTF\0", 4));
    qToLittleEndian<quint32>(7, reinterpret_cast<uchar *>(header.data() + 4));
    qToLittleEndian<quint32>(2, reinterpret_cast<uchar *>(header.data() + 8));
    qToLittleEndian<quint32>(80, reinterpret_cast<uchar *>(header.data() + 12));
    qToLittleEndian<quint16>(image.width(), reinterpret_cast<uchar *>(header.data() + 16));
    qToLittleEndian<quint16>(image.height(), reinterpret_cast<uchar *>(header.data() + 18));
    qToLittleEndian<quint32>(0x100, reinterpret_cast<uchar *>(header.data() + 20));
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar *>(header.data() + 24));
    const float bumpScale = 1.0f;
    memcpy(header.data() + 48, &bumpScale, sizeof(float));
    qToLittleEndian<quint32>(RGBA8888, reinterpret_cast<uchar *>(header.data() + 52));
    header[56] = 1;
    qToLittleEndian<quint32>(quint32(FORMAT_NONE), reinterpret_cast<uchar *>(header.data() + 57));
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar *>(header.data() + 63));
    if (device->write(header) != header.size()) { *error = QStringLiteral("Could not write VTF header"); return false; }
    for (int y = 0; y < image.height(); ++y) {
        const qint64 bytes = image.width() * 4;
        if (device->write(reinterpret_cast<const char *>(image.constScanLine(y)), bytes) != bytes) {
            *error = QStringLiteral("Could not write VTF image data"); return false;
        }
    }
    return true;
}

} // namespace VtfCodec
