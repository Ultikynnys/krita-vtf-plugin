// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <KisImportExportFilter.h>

class KisVtfImport : public KisImportExportFilter
{
    Q_OBJECT
public:
    KisVtfImport(QObject *parent, const QVariantList &);
    KisImportExportErrorCode convert(KisDocument *document, QIODevice *io,
                                     KisPropertiesConfigurationSP configuration = nullptr) override;
};
