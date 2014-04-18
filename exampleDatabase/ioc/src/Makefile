TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DBD += exampleDatabase.dbd

exampleDatabase_DBD += base.dbd
exampleDatabase_DBD += PVAServerRegister.dbd
exampleDatabase_DBD += registerChannelProviderLocal.dbd
exampleDatabase_DBD += dbPv.dbd
exampleDatabase_DBD += powerSupplyRegister.dbd
exampleDatabase_DBD += exampleDatabaseRegister.dbd

PROD_IOC += exampleDatabase

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
exampleDatabase_SRCS += exampleDatabase_registerRecordDeviceDriver.cpp
exampleDatabase_SRCS_DEFAULT += exampleDatabaseMain.cpp
exampleDatabase_SRCS_vxWorks += -nil-

# The following adds support from base/src/vxWorks
exampleDatabase_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

exampleDatabase_LIBS += exampleDatabase
exampleDatabase_LIBS += powerSupply
exampleDatabase_LIBS += pvDatabase
exampleDatabase_LIBS += pvaSrv
exampleDatabase_LIBS += pvAccess
exampleDatabase_LIBS += pvData
exampleDatabase_LIBS += Com
exampleDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

