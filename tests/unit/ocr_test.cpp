#include "infrastructure/ocr/TesseractOcrProvider.h"

#include <QtTest>

class OcrTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesProviderMetadata()
    {
        Lumina::Infrastructure::TesseractOcrProvider provider;
        QCOMPARE(provider.name(), QStringLiteral("Tesseract OCR"));
        QVERIFY(!provider.busy());
    }

    void rejectsInvalidImageWithoutStarting()
    {
        Lumina::Infrastructure::TesseractOcrProvider provider;
        QVERIFY(!provider.recognize(QImage(), QStringLiteral("eng"),
                                    [](const Lumina::Domain::OcrProvider::Result &, const QString &) {}));
        QVERIFY(!provider.busy());
    }
};

QTEST_GUILESS_MAIN(OcrTest)

#include "ocr_test.moc"
