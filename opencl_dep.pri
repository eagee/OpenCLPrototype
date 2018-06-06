QT += concurrent
INCLUDEPATH += $$(CUDA_PATH)\include
DEPENDPATH += $$(CUDA_PATH)\include
win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += -L$$(CUDA_PATH)\lib\Win32\ -lOpenCL
        PRE_TARGETDEPS += $$(CUDA_PATH)\lib\Win32\OpenCL.lib
    } else {
        LIBS += -L$$(CUDA_PATH)\lib\x64\ -lOpenCL
        PRE_TARGETDEPS += $$(CUDA_PATH)\lib\x64\OpenCL.lib
    }
}
