TOP=../../../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBD += exampleCounter.dbd

LIBRARY_IOC += exampleCounterSupport
exampleCounterSupport_SRCS += exampleCounter.cpp
exampleCounterSupport_LIBS += pvData
exampleCounterSupport_LIBS += pvAccess
exampleCounterSupport_LIBS += pvDatabase
exampleCounterSupport_LIBS += pvDatabaseExample
exampleCounterSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# build an ioc application

PROD_IOC += exampleCounter


# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
exampleCounter_SRCS += exampleCounter_registerRecordDeviceDriver.cpp
exampleCounter_SRCS_DEFAULT += exampleCounterMain.cpp
exampleCounter_SRCS_vxWorks += -nil-


# The following adds support from base/src/vxWorks
exampleCounter_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

exampleCounter_LIBS += pvData pvAccess
exampleCounter_LIBS += pvDatabase
exampleCounter_LIBS += pvDatabaseExample
exampleCounter_LIBS += exampleCounterSupport
exampleCounter_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

