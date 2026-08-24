// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <KisImportExportFilter.h>

class KisVtfExport : public KisImportExportFilter
{
    Q_OBJECT
public:
    KisVtfExport(QObject *parent, const QVariantList &);
    KisImportExportErrorCode convert(KisDocument *document, QIODevice *io,
                                     KisPropertiesConfigurationSP configuration = nullptr) override;
    void initializeCapabilities() override;
    KisPropertiesConfigurationSP defaultConfiguration(const QByteArray &from, const QByteArray &to) const override;
    KisConfigWidget *createConfigurationWidget(QWidget *parent, const QByteArray &from, const QByteArray &to) const override;
};
