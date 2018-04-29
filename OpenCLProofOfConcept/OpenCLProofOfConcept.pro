TEMPLATE = app

QT += qml quick
CONFIG += c++11

include(..\opencl_dep.pri)

INCLUDEPATH += ..\QtOpenCL
DEPENDPATH += ..\QtOpenCL


SOURCES += main.cpp \
    TestScanner.cpp \
    DeviceBoundScanOperation.cpp \
    CPUBoundScanOperations.cpp \
    OpenClProgram.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    TestScanner.h \
    DeviceBoundScanOperation.h \
    CPUBoundScanOperations.h \
    OpenClProgram.h

win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        Release: LIBS += -L../lib/Win32/Release -lQtOpenCL
        Release: PRE_TARGETDEPS += ../lib/Win32/Release/QtOpenCL.lib
        Debug: LIBS += -L../lib/Win32/Debug -lQtOpenCL
        Debug: PRE_TARGETDEPS += ../lib/Win32/Debug/QtOpenCL.lib
    } else {
        Release: LIBS += -L../lib/x64/Release/ -lQtOpenCL
        Release: PRE_TARGETDEPS += ../lib/x64/Release/QtOpenCL.lib
        Debug: LIBS += -L../lib/x64/Debug/ -lQtOpenCL
        Debug: PRE_TARGETDEPS += ../lib/x64/Debug/QtOpenCL.lib
    }
}

#Compiler Flags
# /GL Whole program optimization
# /Zc:strictStrings- MsHTML.h never heard about const correctness
# /LTCG Link-Time Code Generation
# /EHa Allows us to catch hardware exceptions (e.g. access violations) with C++ code
# -msse -msse2 <- TODO: Look up switches for these, see if we need them
QMAKE_CXXFLAGS_RELEASE += /GL
QMAKE_LFLAGS_RELEASE += /LTCG
QMAKE_CXXFLAGS_EXCEPTIONS_ON = /EHa
QMAKE_CXXFLAGS_STL_ON = /EHa
