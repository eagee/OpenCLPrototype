#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "TestScanner.h"
#include <CL/cl.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    auto rootContext = engine.rootContext();

    TestScannerListModel testScannerModel(nullptr);
    rootContext->setContextProperty("qApp", &app);
    rootContext->setContextProperty("testScannerModel", &testScannerModel);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
