// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_export.h"
#include "kis_vtf_codec.h"
#include "kis_wdg_options_vtf.h"

#include <kpluginfactory.h>
#include <KoColorModelStandardIds.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <kis_transparency_mask.h>

#include <limits>

K_PLUGIN_FACTORY_WITH_JSON(KisVtfExportFactory, "krita_vtf_export.json", registerPlugin<KisVtfExport>();)

namespace {

void disableTransparencyMasks(const KisNodeSP &node)
{
    if (dynamic_cast<KisTransparencyMask *>(node.data())) {
        node->setVisible(false);
    }
    for (KisNodeSP child = node->firstChild(); child; child = child->nextSibling()) {
        disableTransparencyMasks(child);
    }
}

} // namespace

KisVtfExport::KisVtfExport(QObject *parent, const QVariantList &)
    : KisImportExportFilter(parent)
{
}

KisImportExportErrorCode KisVtfExport::convert(KisDocument *document, QIODevice *io,
                                                KisPropertiesConfigurationSP configuration)
{
    if (!configuration) configuration = defaultConfiguration(QByteArray(), QByteArray());
    const QRect bounds = document->savingImage()->bounds();
    const qint64 byteCount = qint64(bounds.width()) * bounds.height() * 4;
    if (bounds.isEmpty() || byteCount > std::numeric_limits<int>::max()) {
        setErrorMessage(QStringLiteral("The image is too large to export as VTF"));
        return ImportExportCodes::ErrorWhileWriting;
    }

    // Alpha comes from the normal projection. RGB comes from an independent clone
    // composited with transparency masks disabled, so the document is never modified.
    const KisImageSP alphaImage = document->savingImage();
    QByteArray alphaPixels(int(byteCount), Qt::Uninitialized);
    alphaImage->projection()->readBytes(reinterpret_cast<quint8 *>(alphaPixels.data()), bounds);

    const KisImageSP colorImage = alphaImage->clone(true);
    disableTransparencyMasks(colorImage->rootLayer());
    colorImage->rootLayer()->setDirty(bounds);
    colorImage->waitForDone();
    QByteArray colorPixels(int(byteCount), Qt::Uninitialized);
    colorImage->projection()->readBytes(reinterpret_cast<quint8 *>(colorPixels.data()), bounds);

    QImage image;
    QString error;
    if (!VtfCodec::combineBgra8888ColorAndAlpha(colorPixels, alphaPixels, bounds.size(), &image, &error)) {
        setErrorMessage(error);
        return ImportExportCodes::ErrorWhileWriting;
    }
    VtfCodec::WriteOptions options;
    options.minorVersion = configuration->getInt("versionMinor", 2);
    options.imageFormat = VtfCodec::ImageFormat(configuration->getInt("imageFormat", VtfCodec::DXT5));
    options.flags = quint32(configuration->getInt("textureFlags", VtfCodec::Trilinear));
    options.generateMipmaps = configuration->getBool("generateMipmaps", true);
    options.generateThumbnail = configuration->getBool("generateThumbnail", true);
    options.thumbnailSize = quint8(configuration->getInt("thumbnailSize", 16));
    options.bumpScale = float(configuration->getDouble("bumpScale", 1.0));
    if (!VtfCodec::write(io, image, options, &error)) {
        setErrorMessage(error);
        return ImportExportCodes::ErrorWhileWriting;
    }
    return ImportExportCodes::OK;
}

KisPropertiesConfigurationSP KisVtfExport::defaultConfiguration(const QByteArray &, const QByteArray &) const
{
    auto configuration = KisPropertiesConfigurationSP(new KisPropertiesConfiguration);
    configuration->setProperty("versionMinor", 2);
    configuration->setProperty("imageFormat", int(VtfCodec::DXT5));
    configuration->setProperty("textureFlags", quint32(VtfCodec::Trilinear));
    configuration->setProperty("generateMipmaps", true);
    configuration->setProperty("generateThumbnail", true);
    configuration->setProperty("thumbnailSize", 16);
    configuration->setProperty("bumpScale", 1.0);
    return configuration;
}

KisConfigWidget *KisVtfExport::createConfigurationWidget(QWidget *parent, const QByteArray &, const QByteArray &) const
{
    return new KisWdgOptionsVtf(parent);
}

void KisVtfExport::initializeCapabilities()
{
    QList<QPair<KoID, KoID>> supported;
    supported << QPair<KoID, KoID>(RGBAColorModelID, Integer8BitsColorDepthID);
    addSupportedColorModels(supported, QStringLiteral("VTF"));
}

#include "kis_vtf_export.moc"
