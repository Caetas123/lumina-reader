#include "infrastructure/image/ImageCodec.h"

#include <QFileInfo>
#include <QtTest>

class ImageCodecTest final : public QObject {
    Q_OBJECT

private slots:
    void loadsConfiguredImageFixtures()
    {
        const QString webp = qEnvironmentVariable("LUMINA_TEST_WEBP");
        const QString avif = qEnvironmentVariable("LUMINA_TEST_AVIF");
        if (webp.isEmpty() || avif.isEmpty()) {
            QSKIP("Set LUMINA_TEST_WEBP and LUMINA_TEST_AVIF to run the codec fixtures.");
        }

        QVERIFY(QFileInfo(webp).isFile());
        QVERIFY(QFileInfo(avif).isFile());
        const QImage webpImage = Lumina::Infrastructure::loadImageFile(webp);
        const QImage avifImage = Lumina::Infrastructure::loadImageFile(avif);
        QVERIFY(!webpImage.isNull());
        QVERIFY(!avifImage.isNull());
    }
};

QTEST_GUILESS_MAIN(ImageCodecTest)

#include "image_codec_test.moc"
