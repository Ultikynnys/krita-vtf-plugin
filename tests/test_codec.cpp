// SPDX-License-Identifier: GPL-2.0-or-later
#include "kis_vtf_codec.h"

#include <QBuffer>
#include <QtTest>
#include <QtEndian>

class TestVtfCodec : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void versionsAndFormats_data();
    void versionsAndFormats();
    void dxtLabelsDescribeAlpha();
    void bgraConversionPreservesTransparentRgb();
    void mipmapsAndFlags();
    void rejectsInvalidCombinations();
};

void TestVtfCodec::versionsAndFormats_data()
{
    QTest::addColumn<int>("minor");
    QTest::addColumn<int>("format");
    const int formats[] = {VtfCodec::RGBA8888, VtfCodec::RGB888, VtfCodec::RGB565,
                           VtfCodec::I8, VtfCodec::IA88, VtfCodec::DXT1,
                           VtfCodec::DXT1_ONEBITALPHA, VtfCodec::DXT3, VtfCodec::DXT5};
    for (int minor = 0; minor <= 5; ++minor) {
        for (int format : formats) {
            const QByteArray name = QByteArray::number(minor) + '-' + QByteArray::number(format);
            QTest::newRow(name.constData()) << minor << format;
        }
    }
}

void TestVtfCodec::versionsAndFormats()
{
    QFETCH(int, minor); QFETCH(int, format);
    QImage image(8, 8, QImage::Format_RGBA8888);
    image.fill(qRgba(40, 100, 200, 180));
    VtfCodec::WriteOptions options;
    options.minorVersion = minor;
    options.imageFormat = VtfCodec::ImageFormat(format);
    QByteArray bytes; QBuffer buffer(&bytes); QVERIFY(buffer.open(QIODevice::WriteOnly));
    QString error; QVERIFY2(VtfCodec::write(&buffer, image, options, &error), qPrintable(error));
    QCOMPARE(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(bytes.constData() + 8)), quint32(minor));
    QCOMPARE(qint32(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(bytes.constData() + 52))), qint32(format));
    QBuffer input(&bytes); QVERIFY(input.open(QIODevice::ReadOnly));
    QImage decoded; QVERIFY2(VtfCodec::read(&input, &decoded, &error), qPrintable(error));
    QCOMPARE(decoded.size(), image.size());
}

void TestVtfCodec::dxtLabelsDescribeAlpha()
{
    QCOMPARE(VtfCodec::formatName(VtfCodec::DXT1), QStringLiteral("DXT1 (no alpha)"));
    QCOMPARE(VtfCodec::formatName(VtfCodec::DXT1_ONEBITALPHA), QStringLiteral("DXT1 (1-bit alpha)"));
    QCOMPARE(VtfCodec::formatName(VtfCodec::DXT3), QStringLiteral("DXT3 (4-bit explicit alpha)"));
    QCOMPARE(VtfCodec::formatName(VtfCodec::DXT5), QStringLiteral("DXT5 (interpolated alpha)"));
}

void TestVtfCodec::bgraConversionPreservesTransparentRgb()
{
    const QByteArray bgra("\xcf\x65\x17\x00", 4);
    QImage image;
    QString error;
    QVERIFY2(VtfCodec::bgra8888ToRgba8888(bgra, QSize(1, 1), &image, &error), qPrintable(error));
    QCOMPARE(image.format(), QImage::Format_RGBA8888);
    QCOMPARE(image.pixel(0, 0), qRgba(23, 101, 207, 0));

    QVERIFY(!VtfCodec::bgra8888ToRgba8888(QByteArray(3, '\0'), QSize(1, 1), &image, &error));
    QVERIFY(!error.isEmpty());
}

void TestVtfCodec::mipmapsAndFlags()
{
    QImage image(8, 4, QImage::Format_RGBA8888); image.fill(Qt::red);
    VtfCodec::WriteOptions options; options.generateMipmaps = true; options.flags = VtfCodec::ClampS;
    options.generateThumbnail = true; options.thumbnailSize = 4;
    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); QString error;
    QVERIFY2(VtfCodec::write(&buffer, image, options, &error), qPrintable(error));
    QCOMPARE(quint8(bytes[56]), quint8(4));
    QCOMPARE(quint8(bytes[61]), quint8(4));
    const quint32 flags = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(bytes.constData() + 20));
    QVERIFY(flags & VtfCodec::ClampS); QVERIFY(!(flags & VtfCodec::NoMipmaps));
}

void TestVtfCodec::rejectsInvalidCombinations()
{
    QImage image(7, 8, QImage::Format_RGBA8888); image.fill(Qt::black);
    VtfCodec::WriteOptions options; options.generateMipmaps = true; QString error;
    QVERIFY(!VtfCodec::validate(image, options, &error)); QVERIFY(!error.isEmpty());
    options.generateMipmaps = false; options.minorVersion = 6;
    QVERIFY(!VtfCodec::validate(image, options, &error));
}

QTEST_MAIN(TestVtfCodec)
#include "test_codec.moc"
