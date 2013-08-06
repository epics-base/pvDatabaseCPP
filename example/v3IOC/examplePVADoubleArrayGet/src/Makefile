TOP=../../../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBD += examplePVADoubleArrayGet.dbd

LIBRARY_IOC += examplePVADoubleArrayGetSupport
examplePVADoubleArrayGetSupport_SRCS += examplePVADoubleArrayGet.cpp
examplePVADoubleArrayGetSupport_LIBS += pvData
examplePVADoubleArrayGetSupport_LIBS += pvAccess
examplePVADoubleArrayGetSupport_LIBS += pvDatabase
examplePVADoubleArrayGetSupport_LIBS += pvDatabaseExample
examplePVADoubleArrayGetSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# build an ioc application

PROD_IOC += examplePVADoubleArrayGet


# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
examplePVADoubleArrayGet_SRCS += examplePVADoubleArrayGet_registerRecordDeviceDriver.cpp
examplePVADoubleArrayGet_SRCS_DEFAULT += examplePVADoubleArrayGetMain.cpp
examplePVADoubleArrayGet_SRCS_vxWorks += -nil-


# The following adds support from base/src/vxWorks
examplePVADoubleArrayGet_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

examplePVADoubleArrayGet_LIBS += pvData pvAccess
examplePVADoubleArrayGet_LIBS += pvDatabase
examplePVADoubleArrayGet_LIBS += pvDatabaseExample
examplePVADoubleArrayGet_LIBS += examplePVADoubleArrayGetSupport
examplePVADoubleArrayGet_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

