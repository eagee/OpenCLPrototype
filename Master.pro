TEMPLATE = subdirs
SUBDIRS = QtOpenCL OpenCLProofOfConcept
CONFIG += ordered

OpenCLProofOfConcept.depends = QtOpenCL

include(doc/doc.pri)
