TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBD += examplePVADoubleArrayGet.dbd

INC += examplePVADoubleArrayGet.h

LIBRARY_IOC += examplePVADoubleArrayGetSupport
examplePVADoubleArrayGetSupport_SRCS += examplePVADoubleArrayGet.cpp
examplePVADoubleArrayGetSupport_SRCS += examplePVADoubleArrayGetRegister.cpp
examplePVADoubleArrayGetSupport_LIBS += pvData
examplePVADoubleArrayGetSupport_LIBS += pvAccess
examplePVADoubleArrayGetSupport_LIBS += pvDatabase
examplePVADoubleArrayGetSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

