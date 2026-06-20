#include "core/modeling/model_import_manager.h"
#include "core/modeling/model_property_service.h"

#include <QtTest>

namespace {

class ModelingServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void detectsSupportedExtensions();
    void importUnsupportedFileReturnsInvalidDocument();
    void formatsMeasureSummary();
};

void ModelingServicesTest::detectsSupportedExtensions()
{
    pyraqt::core::ModelImportManager manager;

    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.py")), pyraqt::core::ModelFormat::Unknown);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.stp")), pyraqt::core::ModelFormat::Step);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.step")), pyraqt::core::ModelFormat::Step);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.brep")), pyraqt::core::ModelFormat::Brep);
    QCOMPARE(manager.detectFormat(QStringLiteral("/tmp/sample.txt")), pyraqt::core::ModelFormat::Unknown);

    QVERIFY(manager.isSupportedFile(QStringLiteral("/tmp/sample.stp")));
    QVERIFY(manager.isSupportedFile(QStringLiteral("/tmp/sample.step")));
    QVERIFY(manager.isSupportedFile(QStringLiteral("/tmp/sample.brep")));
    QVERIFY(!manager.isSupportedFile(QStringLiteral("/tmp/sample.py")));
}

void ModelingServicesTest::importUnsupportedFileReturnsInvalidDocument()
{
    pyraqt::core::ModelImportManager manager;
    const pyraqt::core::ModelDocument document = manager.importFile(QStringLiteral("/tmp/sample.txt"));
    QCOMPARE(document.format, pyraqt::core::ModelFormat::Unknown);
    QVERIFY(!document.isValid);
    QVERIFY(!document.summary.isValid);
    QVERIFY(!document.summary.errorMessage.isEmpty());
}

void ModelingServicesTest::formatsMeasureSummary()
{
    pyraqt::core::ModelMeasurementSummary summary;
    summary.hasLength = true;
    summary.length = 12.5;
    summary.hasArea = true;
    summary.area = 42.75;
    summary.centerOfMassText = QStringLiteral("[1.000, 2.000, 3.000]");

    const QString text = pyraqt::core::ModelPropertyService::formatMeasureSummary(summary);
    QVERIFY(text.contains(QStringLiteral("L=12.500")));
    QVERIFY(text.contains(QStringLiteral("A=42.750")));
    QVERIFY(text.contains(QStringLiteral("COM=[1.000, 2.000, 3.000]")));
}

} // namespace

QObject *createModelingServicesTest()
{
    return new ModelingServicesTest();
}

#include "test_modeling_services.moc"
