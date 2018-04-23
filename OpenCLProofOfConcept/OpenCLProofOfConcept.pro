TEMPLATE = app

QT += qml quick
CONFIG += c++11

include(..\opencl_dep.pri)

INCLUDEPATH += ..\QtOpenCL
DEPENDPATH += ..\QtOpenCL


SOURCES += main.cpp \
    TestScanner.cpp \
    CPUBoundScanObject.cpp \
    DeviceBoundScanOperation.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    TestScanner.h \
    CPUBoundScanObject.h \
    DeviceBoundScanOperation.h

#Compiler Flags
# /GL Whole program optimization
# /Zc:strictStrings- MsHTML.h never heard about const correctness
# /LTCG Link-Time Code Generation
# /EHa Allows us to catch hardware exceptions (e.g. access violations) with C++ code
QMAKE_CXXFLAGS_RELEASE += /GL -msse -msse2
QMAKE_LFLAGS_RELEASE += /LTCG
QMAKE_CXXFLAGS_EXCEPTIONS_ON = /EHa
QMAKE_CXXFLAGS_STL_ON = /EHa
