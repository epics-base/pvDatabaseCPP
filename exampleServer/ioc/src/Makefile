TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DBD += exampleServer.dbd

exampleServer_DBD += base.dbd
exampleServer_DBD += PVAServerRegister.dbd
exampleServer_DBD += registerChannelProviderLocal.dbd
exampleServer_DBD += dbPv.dbd
exampleServer_DBD += exampleServerRegister.dbd

PROD_IOC += exampleServer

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
exampleServer_SRCS += exampleServer_registerRecordDeviceDriver.cpp
exampleServer_SRCS_DEFAULT += exampleServerMain.cpp
exampleServer_SRCS_vxWorks += -nil-

# The following adds support from base/src/vxWorks
exampleServer_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

exampleServer_LIBS += exampleServer
exampleServer_LIBS += pvDatabase
exampleServer_LIBS += pvaSrv
exampleServer_LIBS += pvAccess
exampleServer_LIBS += pvData
exampleServer_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

