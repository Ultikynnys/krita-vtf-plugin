// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_codec.h"

#include <QImageIOHandler>
#include <QImageIOPlugin>

class VtfHandler final : public QImageIOHandler
{
public:
    bool canRead() const override
    {
        if (!device()) return false;
        return device()->peek(4) == QByteArray("VTF\0", 4);
    }

    bool read(QImage *image) override
    {
        QString error;
        return VtfCodec::read(device(), image, &error);
    }

    bool write(const QImage &image) override
    {
        QString error;
        return VtfCodec::write(device(), image, &error);
    }

    QVariant option(ImageOption option) const override
    {
        if (option != Size || !device()) return QVariant();
        const QByteArray header = device()->peek(20);
        if (header.size() < 20 || header.left(4) != QByteArray("VTF\0", 4)) return QVariant();
        const uchar *bytes = reinterpret_cast<const uchar *>(header.constData());
        const int width = bytes[16] | (bytes[17] << 8);
        const int height = bytes[18] | (bytes[19] << 8);
        return QSize(width, height);
    }

    bool supportsOption(ImageOption option) const override
    {
        return option == Size;
    }
};

class VtfPlugin final : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "vtf.json")
public:
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const override
    {
        if (format.toLower() == "vtf") return CanRead | CanWrite;
        if (device && device->isReadable() && device->peek(4) == QByteArray("VTF\0", 4)) return CanRead;
        return {};
    }

    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const override
    {
        auto *handler = new VtfHandler;
        handler->setDevice(device);
        handler->setFormat(format.isEmpty() ? QByteArray("vtf") : format);
        return handler;
    }
};

#include "kis_vtf_qimageio.moc"
