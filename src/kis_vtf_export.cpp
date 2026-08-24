// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_export.h"
#include "kis_vtf_codec.h"

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
    Q_UNUSED(configuration);
    const QRect bounds = document->savingImage()->bounds();
    const QImage image = document->savingImage()->projection()->convertToQImage(
        nullptr, 0, 0, bounds.width(), bounds.height(),
        KoColorConversionTransformation::internalRenderingIntent(),
        KoColorConversionTransformation::internalConversionFlags());
    QString error;
    if (!VtfCodec::write(io, image, &error)) {
        setErrorMessage(error);
        return ImportExportCodes::ErrorWhileWriting;
    }
    return ImportExportCodes::OK;
}

void KisVtfExport::initializeCapabilities()
{
    QList<QPair<KoID, KoID>> supported;
    supported << QPair<KoID, KoID>(RGBAColorModelID, Integer8BitsColorDepthID);
    addSupportedColorModels(supported, QStringLiteral("VTF"));
}

#include "kis_vtf_export.moc"
