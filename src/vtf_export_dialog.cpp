// SPDX-License-Identifier: GPL-2.0-or-later
#include "vtf_export_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
struct FlagControl { const char *key; const char *label; quint32 flag; bool usage; };
const FlagControl controls[] = {
    {"pointSample", "Point sampling", VtfCodec::PointSample, false}, {"trilinear", "Trilinear filtering", VtfCodec::Trilinear, false},
    {"clampS", "Clamp S", VtfCodec::ClampS, false}, {"clampT", "Clamp T", VtfCodec::ClampT, false},
    {"clampU", "Clamp U", VtfCodec::ClampU, false}, {"anisotropic", "Anisotropic filtering", VtfCodec::Anisotropic, false},
    {"border", "Border", VtfCodec::Border, false}, {"hintDxt5", "Hint DXT5", VtfCodec::HintDxt5, true},
    {"pwlCorrected", "PWL corrected", VtfCodec::PwlCorrected, true}, {"normalMap", "Normal map", VtfCodec::NormalMap, true},
    {"noLod", "No LOD", VtfCodec::NoLod, true}, {"allMips", "All mips", VtfCodec::AllMips, true},
    {"procedural", "Procedural", VtfCodec::Procedural, true}, {"environmentMap", "Environment map", VtfCodec::EnvironmentMap, true},
    {"renderTarget", "Render target", VtfCodec::RenderTarget, true}, {"depthRenderTarget", "Depth render target", VtfCodec::DepthRenderTarget, true},
    {"noDebugOverride", "No debug override", VtfCodec::NoDebugOverride, true}, {"singleCopy", "Single copy", VtfCodec::SingleCopy, true},
    {"preSrgb", "Pre-sRGB", VtfCodec::PreSrgb, true}, {"noDepthBuffer", "No depth buffer", VtfCodec::NoDepthBuffer, true},
    {"vertexTexture", "Vertex texture", VtfCodec::VertexTexture, true}, {"ssBump", "SSBump", VtfCodec::SsBump, true}
};
QCheckBox *box(const QGroupBox *group, const char *key) { return group->findChild<QCheckBox *>(QString::fromLatin1(key)); }
}

VtfExportDialog::VtfExportDialog(const QImage &image, QWidget *parent)
    : QDialog(parent), m_image(image)
{
    setWindowTitle(QStringLiteral("VTF Export Options"));
    resize(540, 620);
    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_version = new QComboBox(this); for (int minor = 0; minor <= 5; ++minor) m_version->addItem(QStringLiteral("VTF 7.%1").arg(minor), minor);
    m_format = new QComboBox(this);
    const VtfCodec::ImageFormat formats[] = {VtfCodec::DXT1, VtfCodec::DXT1_ONEBITALPHA, VtfCodec::DXT3, VtfCodec::DXT5,
        VtfCodec::RGBA8888, VtfCodec::BGRA8888, VtfCodec::ABGR8888, VtfCodec::ARGB8888, VtfCodec::RGB888,
        VtfCodec::BGR888, VtfCodec::BGRX8888, VtfCodec::RGB565, VtfCodec::I8, VtfCodec::IA88, VtfCodec::A8};
    for (auto format : formats) m_format->addItem(VtfCodec::formatName(format), int(format));
    m_mipmaps = new QCheckBox(QStringLiteral("Generate complete mipmap chain"), this);
    m_thumbnail = new QCheckBox(QStringLiteral("Generate DXT1 thumbnail"), this);
    m_thumbnailSize = new QSpinBox(this); m_thumbnailSize->setRange(1, 64);
    m_bumpScale = new QDoubleSpinBox(this); m_bumpScale->setRange(0, 1000); m_bumpScale->setDecimals(3);
    form->addRow(QStringLiteral("Version:"), m_version); form->addRow(QStringLiteral("Image format:"), m_format);
    form->addRow(m_mipmaps); form->addRow(m_thumbnail); form->addRow(QStringLiteral("Thumbnail size:"), m_thumbnailSize);
    form->addRow(QStringLiteral("Bump-map scale:"), m_bumpScale); root->addLayout(form);
    m_filterFlags = new QGroupBox(QStringLiteral("Sampling and wrapping flags"), this);
    m_usageFlags = new QGroupBox(QStringLiteral("Texture usage flags"), this);
    auto *filterLayout = new QGridLayout(m_filterFlags); auto *usageLayout = new QGridLayout(m_usageFlags);
    const int filterCols = 2, usageCols = 3;
    int fCol = 0, fRow = 0, uCol = 0, uRow = 0;
    for (const auto &control : controls) {
        auto *check = new QCheckBox(QString::fromLatin1(control.label), control.usage ? m_usageFlags : m_filterFlags);
        check->setObjectName(QString::fromLatin1(control.key));
        if (control.usage) { usageLayout->addWidget(check, uRow, uCol); if (++uCol == usageCols) { uCol = 0; ++uRow; } }
        else               { filterLayout->addWidget(check, fRow, fCol); if (++fCol == filterCols) { fCol = 0; ++fRow; } }
        connect(check, &QCheckBox::toggled, this, &VtfExportDialog::validateOptions);
    }
    root->addWidget(m_filterFlags); root->addWidget(m_usageFlags);
    m_validation = new QLabel(this); m_validation->setWordWrap(true); root->addWidget(m_validation);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_ok = buttons->button(QDialogButtonBox::Ok); root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QSettings settings(QStringLiteral("KritaVtfPlugin"), QStringLiteral("Export"));
    m_version->setCurrentIndex(m_version->findData(settings.value("versionMinor", 2).toInt()));
    m_format->setCurrentIndex(m_format->findData(settings.value("imageFormat", int(VtfCodec::DXT5)).toInt()));
    m_mipmaps->setChecked(settings.value("generateMipmaps", true).toBool()); m_thumbnail->setChecked(settings.value("generateThumbnail", true).toBool());
    m_thumbnailSize->setValue(settings.value("thumbnailSize", 16).toInt()); m_bumpScale->setValue(settings.value("bumpScale", 1.0).toDouble());
    const quint32 flags = settings.value("textureFlags", quint32(VtfCodec::Trilinear)).toUInt();
    for (const auto &control : controls) box(control.usage ? m_usageFlags : m_filterFlags, control.key)->setChecked(flags & control.flag);
    connect(m_version, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VtfExportDialog::validateOptions);
    connect(m_format, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VtfExportDialog::validateOptions);
    connect(m_mipmaps, &QCheckBox::toggled, this, &VtfExportDialog::validateOptions);
    connect(m_thumbnail, &QCheckBox::toggled, this, &VtfExportDialog::validateOptions);
    connect(m_thumbnail, &QCheckBox::toggled, m_thumbnailSize, &QWidget::setEnabled);
    validateOptions();
}

VtfCodec::WriteOptions VtfExportDialog::currentOptions() const
{
    VtfCodec::WriteOptions options; options.minorVersion = m_version->currentData().toUInt();
    options.imageFormat = VtfCodec::ImageFormat(m_format->currentData().toInt()); options.generateMipmaps = m_mipmaps->isChecked();
    options.generateThumbnail = m_thumbnail->isChecked(); options.thumbnailSize = quint8(m_thumbnailSize->value()); options.bumpScale = float(m_bumpScale->value());
    options.flags = 0; for (const auto &control : controls) if (box(control.usage ? m_usageFlags : m_filterFlags, control.key)->isChecked()) options.flags |= control.flag;
    return options;
}

VtfCodec::WriteOptions VtfExportDialog::options() const
{
    const auto result = currentOptions(); QSettings settings(QStringLiteral("KritaVtfPlugin"), QStringLiteral("Export"));
    settings.setValue("versionMinor", result.minorVersion); settings.setValue("imageFormat", int(result.imageFormat)); settings.setValue("textureFlags", result.flags);
    settings.setValue("generateMipmaps", result.generateMipmaps); settings.setValue("generateThumbnail", result.generateThumbnail);
    settings.setValue("thumbnailSize", result.thumbnailSize); settings.setValue("bumpScale", result.bumpScale); return result;
}

void VtfExportDialog::validateOptions()
{
    QString error; const bool valid = VtfCodec::validate(m_image, currentOptions(), &error);
    m_validation->setText(valid ? QStringLiteral("Settings are valid. Alpha and NOMIP flags are synchronized automatically.") : error);
    m_validation->setStyleSheet(valid ? QString() : QStringLiteral("color: #d03030;")); m_ok->setEnabled(valid);
    m_thumbnailSize->setEnabled(m_thumbnail->isChecked());
}
