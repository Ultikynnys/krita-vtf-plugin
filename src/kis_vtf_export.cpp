// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_export.h"
#include "kis_vtf_codec.h"
#include "kis_wdg_options_vtf.h"

#include <kpluginfactory.h>
#include <KoColorModelStandardIds.h>
#include <KoColorConversionTransformation.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_device.h>

K_PLUGIN_FACTORY_WITH_JSON(KisVtfExportFactory, "krita_vtf_export.json", registerPlugin<KisVtfExport>();)

KisVtfExport::KisVtfExport(QObject *parent, const QVariantList &)
    : KisImportExportFilter(parent)
{
}

KisImportExportErrorCode KisVtfExport::convert(KisDocument *document, QIODevice *io,
                                                KisPropertiesConfigurationSP configuration)
{
    if (!configuration) configuration = defaultConfiguration(QByteArray(), QByteArray());
    const QRect bounds = document->savingImage()->bounds();
    const QImage image = document->savingImage()->projection()->convertToQImage(
        nullptr, 0, 0, bounds.width(), bounds.height(),
        KoColorConversionTransformation::internalRenderingIntent(),
        KoColorConversionTransformation::internalConversionFlags());
    VtfCodec::WriteOptions options;
    options.minorVersion = configuration->getInt("versionMinor", 2);
    options.imageFormat = VtfCodec::ImageFormat(configuration->getInt("imageFormat", VtfCodec::DXT5));
    options.flags = quint32(configuration->getInt("textureFlags", VtfCodec::Trilinear));
    options.generateMipmaps = configuration->getBool("generateMipmaps", true);
    options.generateThumbnail = configuration->getBool("generateThumbnail", true);
    options.thumbnailSize = quint8(configuration->getInt("thumbnailSize", 16));
    options.bumpScale = float(configuration->getDouble("bumpScale", 1.0));
    QString error;
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
