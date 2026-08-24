// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <kis_config_widget.h>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QSpinBox;

class KisWdgOptionsVtf final : public KisConfigWidget
{
    Q_OBJECT
public:
    explicit KisWdgOptionsVtf(QWidget *parent);
    void setConfiguration(KisPropertiesConfigurationSP configuration) override;
    KisPropertiesConfigurationSP configuration() const override;

private:
    QComboBox *m_version;
    QComboBox *m_format;
    QCheckBox *m_mipmaps;
    QCheckBox *m_thumbnail;
    QSpinBox *m_thumbnailSize;
    QDoubleSpinBox *m_bumpScale;
    QGroupBox *m_filterFlags;
    QGroupBox *m_usageFlags;
};
