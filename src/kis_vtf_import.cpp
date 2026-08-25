// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_import.h"
#include "kis_vtf_codec.h"

#include <kpluginfactory.h>
#include <KoColorSpaceRegistry.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>

K_PLUGIN_FACTORY_WITH_JSON(KisVtfImportFactory, "krita_vtf_import.json", registerPlugin<KisVtfImport>();)

KisVtfImport::KisVtfImport(QObject *parent, const QVariantList &)
    : KisImportExportFilter(parent)
{
}

KisImportExportErrorCode KisVtfImport::convert(KisDocument *document, QIODevice *io,
                                                KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(configuration);
    QImage decoded;
    QString error;
    if (!VtfCodec::read(io, &decoded, &error)) {
        return KisImportExportErrorCode(error);
    }

    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(document->createUndoStore(), decoded.width(), decoded.height(),
                                    colorSpace, QStringLiteral("Imported VTF"));
    KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);
    layer->paintDevice()->convertFromQImage(decoded, nullptr, 0, 0);
    image->addNode(layer.data(), image->rootLayer().data());
    document->setCurrentImage(image);
    return ImportExportCodes::OK;
}

#include "kis_vtf_import.moc"
