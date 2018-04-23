TEMPLATE = lib
TARGET = QtOpenCL
CONFIG += staticlib
CONFIG += warn_on
Qt += core

include(..\opencl_dep.pri)

HEADERS += \
     qclbuffer.h \
     qclcommandqueue.h \
     qclcontext.h \
     qcldevice.h \
     qclevent.h \
     qclglobal.h \
     qclimage.h \
     qclimageformat.h \
     qclkernel.h \
     qclmemoryobject.h \
     qclplatform.h \
     qclprogram.h \
     qclsampler.h \
     qcluserevent.h \
     qclvector.h \
     qclworksize.h

SOURCES += \
     qclbuffer.cpp \
     qclcommandqueue.cpp \
      qclcontext.cpp \
     qcldevice.cpp \
     qclevent.cpp \
     qclimage.cpp \
     qclimageformat.cpp \
     qclkernel.cpp \
     qclmemoryobject.cpp \
     qclplatform.cpp \
     qclprogram.cpp \
     qclsampler.cpp \
     qcluserevent.cpp \
     qclvector.cpp \
     qclworksize.cpp

PRIVATE_HEADERS += \
    qclext_p.h

HEADERS += $$PRIVATE_HEADERS
DEFINES += QT_BUILD_CL_LIB

opencl_1_1 {
    DEFINES += QT_OPENCL_1_1
}
DEFINES += CL_USE_DEPRECATED_OPENCL_1_1_APIS


# Put EXEs and PDBs in a structured lib folder
win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        Release:DESTDIR = ../../lib/Win32/Release
        Debug:DESTDIR = ../../lib/Win32/Debug
    } else {
        Release:DESTDIR = ../../lib/x64/Release
        Debug:DESTDIR = ../../lib/x64/Debug
    }
}
