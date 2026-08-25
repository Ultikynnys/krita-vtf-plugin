// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_codec.h"

#include <QDataStream>
#include <QtEndian>

#include <limits>

namespace {

using namespace VtfCodec;

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

QByteArray encodeUncompressed(const QImage &image, ImageFormat format)
{
    QByteArray output;
    const int size = imageSize(image.width(), image.height(), format);
    output.reserve(size);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            const uchar r = qRed(pixel), g = qGreen(pixel), b = qBlue(pixel), a = qAlpha(pixel);
            switch (format) {
            case RGBA8888: output.append(char(r)); output.append(char(g)); output.append(char(b)); output.append(char(a)); break;
            case ABGR8888: output.append(char(a)); output.append(char(b)); output.append(char(g)); output.append(char(r)); break;
            case RGB888: output.append(char(r)); output.append(char(g)); output.append(char(b)); break;
            case BGR888: output.append(char(b)); output.append(char(g)); output.append(char(r)); break;
            case ARGB8888: output.append(char(a)); output.append(char(r)); output.append(char(g)); output.append(char(b)); break;
            case BGRA8888: output.append(char(b)); output.append(char(g)); output.append(char(r)); output.append(char(a)); break;
            case BGRX8888: output.append(char(b)); output.append(char(g)); output.append(char(r)); output.append(char(255)); break;
            case RGB565: {
                const quint16 value = quint16((r >> 3) << 11) | quint16((g >> 2) << 5) | quint16(b >> 3);
                output.append(char(value & 255)); output.append(char(value >> 8)); break;
            }
            case I8: output.append(char((int(r) + int(g) + int(b)) / 3)); break;
            case IA88: {
                output.append(char((int(r) + int(g) + int(b)) / 3)); output.append(char(a)); break;
            }
            case A8: output.append(char(a)); break;
            default: return QByteArray();
            }
        }
    }
    return output;
}

quint16 to565(QRgb color)
{
    return quint16((qRed(color) >> 3) << 11) | quint16((qGreen(color) >> 2) << 5) | quint16(qBlue(color) >> 3);
}

quint32 colorDistance(QRgb a, QRgb b)
{
    const int dr = qRed(a) - qRed(b), dg = qGreen(a) - qGreen(b), db = qBlue(a) - qBlue(b);
    return quint32(dr * dr + dg * dg + db * db);
}

QByteArray encodeColorBlock(const QRgb pixels[16], bool oneBitAlpha)
{
    int minLuma = 100000, maxLuma = -1;
    QRgb dark = qRgb(0, 0, 0), light = qRgb(255, 255, 255);
    for (int i = 0; i < 16; ++i) {
        if (oneBitAlpha && qAlpha(pixels[i]) < 128) continue;
        const int luma = qRed(pixels[i]) * 3 + qGreen(pixels[i]) * 6 + qBlue(pixels[i]);
        if (luma < minLuma) { minLuma = luma; dark = pixels[i]; }
        if (luma > maxLuma) { maxLuma = luma; light = pixels[i]; }
    }
    quint16 c0 = to565(light), c1 = to565(dark);
    if (oneBitAlpha) {
        if (c0 > c1) qSwap(c0, c1);
    } else if (c0 <= c1) {
        qSwap(c0, c1);
        if (c0 == c1 && c0 < 65535) ++c0;
    }
    QRgb palette[4]; quint32 ignored;
    uchar block[8] = {};
    qToLittleEndian(c0, block); qToLittleEndian(c1, block + 2);
    decodeColors(block, palette, &ignored, !oneBitAlpha);
    quint32 indices = 0;
    for (int i = 0; i < 16; ++i) {
        int best = oneBitAlpha && qAlpha(pixels[i]) < 128 ? 3 : 0;
        quint32 bestDistance = best == 3 ? 0 : colorDistance(pixels[i], palette[0]);
        const int limit = oneBitAlpha ? 3 : 4;
        for (int p = 1; p < limit; ++p) {
            const quint32 distance = colorDistance(pixels[i], palette[p]);
            if (distance < bestDistance) { bestDistance = distance; best = p; }
        }
        indices |= quint32(best) << (2 * i);
    }
    qToLittleEndian(indices, block + 4);
    return QByteArray(reinterpret_cast<const char *>(block), 8);
}

QByteArray encodeDxt(const QImage &image, ImageFormat format)
{
    QByteArray output;
    for (int by = 0; by < (image.height() + 3) / 4; ++by) {
        for (int bx = 0; bx < (image.width() + 3) / 4; ++bx) {
            QRgb pixels[16];
            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                pixels[py * 4 + px] = image.pixel(qMin(bx * 4 + px, image.width() - 1),
                                                  qMin(by * 4 + py, image.height() - 1));
            }
            if (format == DXT3) {
                quint64 alpha = 0;
                for (int i = 0; i < 16; ++i) alpha |= quint64(qAlpha(pixels[i]) >> 4) << (4 * i);
                for (int i = 0; i < 8; ++i) output.append(char((alpha >> (8 * i)) & 255));
            } else if (format == DXT5) {
                int minimum = 255, maximum = 0;
                for (const QRgb pixel : pixels) { minimum = qMin(minimum, qAlpha(pixel)); maximum = qMax(maximum, qAlpha(pixel)); }
                output.append(char(maximum)); output.append(char(minimum));
                int palette[8] = {maximum, minimum, 0, 0, 0, 0, 0, 0};
                for (int i = 0; i < 6; ++i) palette[i + 2] = (maximum * (6 - i) + minimum * (i + 1)) / 7;
                quint64 bits = 0;
                for (int i = 0; i < 16; ++i) {
                    int best = 0, distance = 1000;
                    for (int p = 0; p < 8; ++p) if (qAbs(qAlpha(pixels[i]) - palette[p]) < distance) {
                        distance = qAbs(qAlpha(pixels[i]) - palette[p]); best = p;
                    }
                    bits |= quint64(best) << (3 * i);
                }
                for (int i = 0; i < 6; ++i) output.append(char((bits >> (8 * i)) & 255));
            }
            output.append(encodeColorBlock(pixels, format == DXT1_ONEBITALPHA));
        }
    }
    return output;
}

QByteArray encodeImage(const QImage &image, ImageFormat format)
{
    if (format == DXT1 || format == DXT1_ONEBITALPHA || format == DXT3 || format == DXT5) {
        return encodeDxt(image, format);
    }
    return encodeUncompressed(image, format);
}

bool isPowerOfTwo(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

} // namespace

namespace VtfCodec {

bool combineBgra8888ColorAndAlpha(const QByteArray &colorPixels, const QByteArray &alphaPixels,
                                  const QSize &size, QImage *image, QString *error)
{
    if (!image || !error) return false;
    if (!size.isValid()) {
        *error = QStringLiteral("The RGBA8 projection size is invalid");
        return false;
    }
    const qint64 byteCount = qint64(size.width()) * size.height() * 4;
    if (byteCount > std::numeric_limits<int>::max() ||
        colorPixels.size() != byteCount || alphaPixels.size() != byteCount) {
        *error = QStringLiteral("The RGBA8 projection buffers have an invalid size");
        return false;
    }

    *image = QImage(size, QImage::Format_RGBA8888);
    if (image->isNull()) {
        *error = QStringLiteral("Could not allocate the VTF source image");
        return false;
    }
    const quint8 *color = reinterpret_cast<const quint8 *>(colorPixels.constData());
    const quint8 *alpha = reinterpret_cast<const quint8 *>(alphaPixels.constData());
    for (int y = 0; y < size.height(); ++y) {
        quint8 *destination = image->scanLine(y);
        for (int x = 0; x < size.width(); ++x) {
            destination[0] = color[2];
            destination[1] = color[1];
            destination[2] = color[0];
            destination[3] = alpha[3];
            color += 4;
            alpha += 4;
            destination += 4;
        }
    }
    return true;
}

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

QString formatName(ImageFormat format)
{
    switch (format) {
    case RGBA8888: return QStringLiteral("RGBA8888");
    case ABGR8888: return QStringLiteral("ABGR8888");
    case RGB888: return QStringLiteral("RGB888");
    case BGR888: return QStringLiteral("BGR888");
    case RGB565: return QStringLiteral("RGB565");
    case I8: return QStringLiteral("I8");
    case IA88: return QStringLiteral("IA88");
    case A8: return QStringLiteral("A8");
    case ARGB8888: return QStringLiteral("ARGB8888");
    case BGRA8888: return QStringLiteral("BGRA8888");
    case DXT1: return QStringLiteral("DXT1 (no alpha)");
    case DXT3: return QStringLiteral("DXT3 (4-bit explicit alpha)");
    case DXT5: return QStringLiteral("DXT5 (interpolated alpha)");
    case BGRX8888: return QStringLiteral("BGRX8888");
    case DXT1_ONEBITALPHA: return QStringLiteral("DXT1 (1-bit alpha)");
    default: return QStringLiteral("Unknown");
    }
}

bool formatSupportsAlpha(ImageFormat format)
{
    return format == RGBA8888 || format == ABGR8888 || format == IA88 || format == A8 ||
           format == ARGB8888 || format == BGRA8888 || format == DXT1_ONEBITALPHA ||
           format == DXT3 || format == DXT5;
}

bool validate(const QImage &image, const WriteOptions &options, QString *error)
{
    if (image.isNull() || image.width() > 65535 || image.height() > 65535) {
        *error = QStringLiteral("VTF dimensions must be between 1 and 65535 pixels"); return false;
    }
    if (options.minorVersion > 5) {
        *error = QStringLiteral("Supported VTF versions are 7.0 through 7.5"); return false;
    }
    if (imageSize(image.width(), image.height(), options.imageFormat) < 0) {
        *error = QStringLiteral("Encoding %1 is not implemented").arg(formatName(options.imageFormat)); return false;
    }
    if (options.generateMipmaps && (!isPowerOfTwo(image.width()) || !isPowerOfTwo(image.height()))) {
        *error = QStringLiteral("Mipmapped VTF textures must have power-of-two dimensions"); return false;
    }
    if ((options.flags & EnvironmentMap) && (image.width() != image.height() || !isPowerOfTwo(image.width()))) {
        *error = QStringLiteral("Environment maps must be square and power-of-two"); return false;
    }
    if (options.generateThumbnail && (options.thumbnailSize == 0 || options.thumbnailSize > 64)) {
        *error = QStringLiteral("Thumbnail size must be between 1 and 64 pixels"); return false;
    }
    return true;
}

bool write(QIODevice *device, const QImage &source, const WriteOptions &options, QString *error)
{
    if (!device || !device->isWritable()) { *error = QStringLiteral("The VTF stream is not writable"); return false; }
    if (!validate(source, options, error)) return false;
    const QImage image = source.convertToFormat(QImage::Format_RGBA8888);
    QList<QImage> mipmaps;
    mipmaps.append(image);
    if (options.generateMipmaps) {
        while (mipmaps.last().width() > 1 || mipmaps.last().height() > 1) {
            mipmaps.append(mipmaps.last().scaled(qMax(1, mipmaps.last().width() / 2),
                                                 qMax(1, mipmaps.last().height() / 2),
                                                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
    }
    quint32 flags = options.flags;
    if (options.generateMipmaps) flags &= ~quint32(NoMipmaps);
    else flags |= NoMipmaps;
    if (formatSupportsAlpha(options.imageFormat)) {
        flags |= options.imageFormat == DXT1_ONEBITALPHA ? OneBitAlpha : EightBitAlpha;
    }
    const bool thumbnail = options.generateThumbnail;
    const int thumbSize = qMin<int>(options.thumbnailSize, qMin(image.width(), image.height()));
    const QImage lowRes = thumbnail ? image.scaled(thumbSize, thumbSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation) : QImage();
    QByteArray header(80, '\0');
    header.replace(0, 4, QByteArray("VTF\0", 4));
    qToLittleEndian<quint32>(7, reinterpret_cast<uchar *>(header.data() + 4));
    qToLittleEndian<quint32>(options.minorVersion, reinterpret_cast<uchar *>(header.data() + 8));
    qToLittleEndian<quint32>(80, reinterpret_cast<uchar *>(header.data() + 12));
    qToLittleEndian<quint16>(image.width(), reinterpret_cast<uchar *>(header.data() + 16));
    qToLittleEndian<quint16>(image.height(), reinterpret_cast<uchar *>(header.data() + 18));
    qToLittleEndian<quint32>(flags, reinterpret_cast<uchar *>(header.data() + 20));
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar *>(header.data() + 24));
    memcpy(header.data() + 48, &options.bumpScale, sizeof(float));
    qToLittleEndian<quint32>(quint32(options.imageFormat), reinterpret_cast<uchar *>(header.data() + 52));
    header[56] = char(mipmaps.size());
    qToLittleEndian<quint32>(quint32(thumbnail ? DXT1 : FORMAT_NONE), reinterpret_cast<uchar *>(header.data() + 57));
    header[61] = char(thumbnail ? thumbSize : 0);
    header[62] = char(thumbnail ? thumbSize : 0);
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar *>(header.data() + 63));
    if (device->write(header) != header.size()) { *error = QStringLiteral("Could not write VTF header"); return false; }
    if (thumbnail) {
        const QByteArray encoded = encodeImage(lowRes, DXT1);
        if (device->write(encoded) != encoded.size()) { *error = QStringLiteral("Could not write VTF thumbnail"); return false; }
    }
    for (int level = mipmaps.size() - 1; level >= 0; --level) {
        const QByteArray encoded = encodeImage(mipmaps[level], options.imageFormat);
        if (encoded.isEmpty() || device->write(encoded) != encoded.size()) {
            *error = QStringLiteral("Could not encode %1 mip level %2").arg(formatName(options.imageFormat)).arg(level);
            return false;
        }
    }
    return true;
}

bool write(QIODevice *device, const QImage &image, QString *error)
{
    return write(device, image, WriteOptions(), error);
}

} // namespace VtfCodec
