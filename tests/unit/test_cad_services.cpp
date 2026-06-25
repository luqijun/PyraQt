#include "core/cad/cad_import_manager.h"
#include "core/cad/cad_property_service.h"

#include <QtTest>

namespace {

class CadServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void detectsSupportedExtensions();
    void importUnsupportedFileReturnsInvalidDocument();
    void formatsMeasureSummary();
};

void CadServicesTest::detectsSupportedExtensions()
{
    pyraqt::core::CadImportManager manager;

    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.py")), pyraqt::core::CadFormat::Unknown);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.stp")), pyraqt::core::CadFormat::Step);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.step")), pyraqt::core::CadFormat::Step);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.brep")), pyraqt::core::CadFormat::Brep);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.txt")), pyraqt::core::CadFormat::Unknown);

    QVERIFY(manager.isSupportedFile(QStringLiteral("/tmp/sample.stp")));
    QVERIFY(manager.isSupportedFile(QStringLiteral("/tmp/sample.step")));
    QVERIFY(manager.isSupportedFile(QStringLiteral("/tmp/sample.brep")));
    QVERIFY(!manager.isSupportedFile(QStringLiteral("/tmp/sample.py")));
}

void CadServicesTest::importUnsupportedFileReturnsInvalidDocument()
{
    pyraqt::core::CadImportManager manager;
    const pyraqt::core::CadDocument document = manager.importFile(QStringLiteral("/tmp/sample.txt"));
    QCOMPARE(document.format, pyraqt::core::CadFormat::Unknown);
    QVERIFY(!document.isValid);
    QVERIFY(!document.summary.isValid);
    QVERIFY(!document.summary.errorMessage.isEmpty());
}

void CadServicesTest::formatsMeasureSummary()
{
    pyraqt::core::CadMeasurementSummary summary;
    summary.hasLength = true;
    summary.length = 12.5;
    summary.hasArea = true;
    summary.area = 42.75;
    summary.centerOfMassText = QStringLiteral("[1.000, 2.000, 3.000]");

    const QString text = pyraqt::core::CadPropertyService::formatMeasureSummary(summary);
    QVERIFY(text.contains(QStringLiteral("L=12.500")));
    QVERIFY(text.contains(QStringLiteral("A=42.750")));
    QVERIFY(text.contains(QStringLiteral("COM=[1.000, 2.000, 3.000]")));
}

} // namespace

QObject *createCadServicesTest()
{
    return new CadServicesTest();
}

#include "test_cad_services.moc"
