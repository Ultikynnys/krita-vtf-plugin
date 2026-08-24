// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "kis_vtf_codec.h"
#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QSpinBox;

class VtfExportDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit VtfExportDialog(const QImage &image, QWidget *parent = nullptr);
    VtfCodec::WriteOptions options() const;

private Q_SLOTS:
    void validateOptions();

private:
    VtfCodec::WriteOptions currentOptions() const;
    const QImage &m_image;
    QComboBox *m_version;
    QComboBox *m_format;
    QCheckBox *m_mipmaps;
    QCheckBox *m_thumbnail;
    QSpinBox *m_thumbnailSize;
    QDoubleSpinBox *m_bumpScale;
    QGroupBox *m_filterFlags;
    QGroupBox *m_usageFlags;
    class QLabel *m_validation;
    class QPushButton *m_ok;
};
