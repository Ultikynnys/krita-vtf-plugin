// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_wdg_options_vtf.h"
#include "kis_vtf_codec.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <kis_properties_configuration.h>

namespace {
struct FlagControl { const char *key; const char *label; quint32 flag; bool usage; };
const FlagControl controls[] = {
    {"pointSample", "Point sampling", VtfCodec::PointSample, false},
    {"trilinear", "Trilinear filtering", VtfCodec::Trilinear, false},
    {"clampS", "Clamp S", VtfCodec::ClampS, false},
    {"clampT", "Clamp T", VtfCodec::ClampT, false},
    {"clampU", "Clamp U", VtfCodec::ClampU, false},
    {"anisotropic", "Anisotropic filtering", VtfCodec::Anisotropic, false},
    {"border", "Border", VtfCodec::Border, false},
    {"hintDxt5", "Hint DXT5", VtfCodec::HintDxt5, true},
    {"pwlCorrected", "PWL corrected", VtfCodec::PwlCorrected, true},
    {"normalMap", "Normal map", VtfCodec::NormalMap, true},
    {"noLod", "No LOD", VtfCodec::NoLod, true},
    {"allMips", "All mips", VtfCodec::AllMips, true},
    {"procedural", "Procedural", VtfCodec::Procedural, true},
    {"environmentMap", "Environment map", VtfCodec::EnvironmentMap, true},
    {"renderTarget", "Render target", VtfCodec::RenderTarget, true},
    {"depthRenderTarget", "Depth render target", VtfCodec::DepthRenderTarget, true},
    {"noDebugOverride", "No debug override", VtfCodec::NoDebugOverride, true},
    {"singleCopy", "Single copy", VtfCodec::SingleCopy, true},
    {"preSrgb", "Pre-sRGB", VtfCodec::PreSrgb, true},
    {"noDepthBuffer", "No depth buffer", VtfCodec::NoDepthBuffer, true},
    {"vertexTexture", "Vertex texture", VtfCodec::VertexTexture, true},
    {"ssBump", "SSBump", VtfCodec::SsBump, true}
};

QCheckBox *flagBox(const QGroupBox *group, const char *key)
{
    return group->findChild<QCheckBox *>(QString::fromLatin1(key));
}
}

KisWdgOptionsVtf::KisWdgOptionsVtf(QWidget *parent)
    : KisConfigWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_version = new QComboBox(this);
    for (int minor = 0; minor <= 5; ++minor) m_version->addItem(QStringLiteral("VTF 7.%1").arg(minor), minor);
    m_format = new QComboBox(this);
    const VtfCodec::ImageFormat formats[] = {
        VtfCodec::DXT1, VtfCodec::DXT1_ONEBITALPHA, VtfCodec::DXT3, VtfCodec::DXT5,
        VtfCodec::RGBA8888, VtfCodec::BGRA8888, VtfCodec::ABGR8888, VtfCodec::ARGB8888,
        VtfCodec::RGB888, VtfCodec::BGR888, VtfCodec::BGRX8888, VtfCodec::RGB565,
        VtfCodec::I8, VtfCodec::IA88, VtfCodec::A8
    };
    for (auto format : formats) m_format->addItem(VtfCodec::formatName(format), int(format));
    m_mipmaps = new QCheckBox(QStringLiteral("Generate complete mipmap chain"), this);
    m_thumbnail = new QCheckBox(QStringLiteral("Generate DXT1 thumbnail"), this);
    m_thumbnailSize = new QSpinBox(this); m_thumbnailSize->setRange(1, 64); m_thumbnailSize->setValue(16);
    m_bumpScale = new QDoubleSpinBox(this); m_bumpScale->setRange(0.0, 1000.0); m_bumpScale->setDecimals(3); m_bumpScale->setValue(1.0);
    form->addRow(QStringLiteral("Version:"), m_version);
    form->addRow(QStringLiteral("Image format:"), m_format);
    form->addRow(m_mipmaps);
    form->addRow(m_thumbnail);
    form->addRow(QStringLiteral("Thumbnail size:"), m_thumbnailSize);
    form->addRow(QStringLiteral("Bump-map scale:"), m_bumpScale);
    root->addLayout(form);

    m_filterFlags = new QGroupBox(QStringLiteral("Sampling and wrapping flags"), this);
    m_usageFlags = new QGroupBox(QStringLiteral("Texture usage flags"), this);
    auto *filterLayout = new QVBoxLayout(m_filterFlags);
    auto *usageLayout = new QVBoxLayout(m_usageFlags);
    for (const auto &control : controls) {
        auto *box = new QCheckBox(QString::fromLatin1(control.label), control.usage ? m_usageFlags : m_filterFlags);
        box->setObjectName(QString::fromLatin1(control.key));
        (control.usage ? usageLayout : filterLayout)->addWidget(box);
    }
    root->addWidget(m_filterFlags);
    root->addWidget(m_usageFlags);
    root->addWidget(new QLabel(QStringLiteral("Alpha and NOMIP flags are synchronized with the selected format and mipmap setting."), this));
    root->addStretch();
    connect(m_thumbnail, &QCheckBox::toggled, m_thumbnailSize, &QWidget::setEnabled);
}

KisPropertiesConfigurationSP KisWdgOptionsVtf::configuration() const
{
    auto configuration = KisPropertiesConfigurationSP(new KisPropertiesConfiguration);
    configuration->setProperty("versionMinor", m_version->currentData().toInt());
    configuration->setProperty("imageFormat", m_format->currentData().toInt());
    configuration->setProperty("generateMipmaps", m_mipmaps->isChecked());
    configuration->setProperty("generateThumbnail", m_thumbnail->isChecked());
    configuration->setProperty("thumbnailSize", m_thumbnailSize->value());
    configuration->setProperty("bumpScale", m_bumpScale->value());
    quint32 flags = 0;
    for (const auto &control : controls) if (flagBox(control.usage ? m_usageFlags : m_filterFlags, control.key)->isChecked()) flags |= control.flag;
    configuration->setProperty("textureFlags", flags);
    return configuration;
}

void KisWdgOptionsVtf::setConfiguration(KisPropertiesConfigurationSP configuration)
{
    const int version = configuration->getInt("versionMinor", 2);
    m_version->setCurrentIndex(qMax(0, m_version->findData(version)));
    const int format = configuration->getInt("imageFormat", VtfCodec::DXT5);
    m_format->setCurrentIndex(qMax(0, m_format->findData(format)));
    m_mipmaps->setChecked(configuration->getBool("generateMipmaps", true));
    m_thumbnail->setChecked(configuration->getBool("generateThumbnail", true));
    m_thumbnailSize->setValue(configuration->getInt("thumbnailSize", 16));
    m_thumbnailSize->setEnabled(m_thumbnail->isChecked());
    m_bumpScale->setValue(configuration->getDouble("bumpScale", 1.0));
    const quint32 flags = configuration->getInt("textureFlags", VtfCodec::Trilinear);
    for (const auto &control : controls) flagBox(control.usage ? m_usageFlags : m_filterFlags, control.key)->setChecked(flags & control.flag);
}
