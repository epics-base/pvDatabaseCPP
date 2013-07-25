TOP=../../../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBD += exampleServer.dbd

LIBRARY_IOC += exampleServerSupport
exampleServerSupport_SRCS += exampleServer.cpp
exampleServerSupport_LIBS += pvData
exampleServerSupport_LIBS += pvAccess
exampleServerSupport_LIBS += pvDatabase
exampleServerSupport_LIBS += pvDatabaseExample
exampleServerSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# build an ioc application

PROD_IOC += exampleServer


# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
exampleServer_SRCS += exampleServer_registerRecordDeviceDriver.cpp
exampleServer_SRCS_DEFAULT += exampleServerMain.cpp
exampleServer_SRCS_vxWorks += -nil-


# The following adds support from base/src/vxWorks
exampleServer_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

exampleServer_LIBS += pvData pvAccess
exampleServer_LIBS += pvDatabase
exampleServer_LIBS += pvDatabaseExample
exampleServer_LIBS += exampleServerSupport
exampleServer_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

