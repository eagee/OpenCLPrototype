#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
// #include "DeviceBoundScanOperation.h"
// #include "TestScanner.h"
// #include <CL/cl.h>
// 
// int main(int argc, char *argv[])
// {
//     QGuiApplication app(argc, argv);
//     
//     QQmlApplicationEngine engine;
//     
//     auto rootContext = engine.rootContext();
//     
//     TestScannerListModel testScannerModel(nullptr);
//     rootContext->setContextProperty("qApp", &app);
//     rootContext->setContextProperty("testScannerModel", &testScannerModel);
// 
//     engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
// 
//     return app.exec();
// }

// This works as a test main
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "OpenClProgram.h"
#include "DeviceBoundScanOperation.h"

int main(int argc, char *argv[]) {
    
    QGuiApplication app(argc, argv);
    OpenClProgram program(":/createChecksum.cl", "createChecksum", OpenClProgram::DeviceType::GPU);
    DeviceBoundScanOperation operation(program);
    
    if (program.errorState() == OpenClProgram::ErrorState::None)
    {
        operation.queueOperation("C:\\TestFiles\\TEST-TROJAN - Copy (1).EXE");
        operation.run();
    }

    return app.exec();
}

