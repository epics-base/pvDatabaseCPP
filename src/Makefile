TOP = ..
include $(TOP)/configure/CONFIG

DATABASE = $(TOP)/src/

LIBRARY_IOC += pvDatabase
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)
pvDatabase_LIBS += Com pvData pvAccess

SRC_DIRS += $(DATABASE)/database
INC += pvDatabase.h
LIBSRCS += pvRecord.cpp
LIBSRCS += pvDatabase.cpp

SRC_DIRS += $(DATABASE)/pvAccess
INC += channelProviderLocal.h
INC += pvCopy.h
INC += monitorAlgorithm.h
DBD += registerChannelProviderLocal.dbd
LIBSRCS += channelProviderLocal.cpp
LIBSRCS += pvCopy.cpp
LIBSRCS += monitorFactory.cpp
LIBSRCS += channelLocal.cpp
LIBSRCS += registerChannelProviderLocal.cpp

SRC_DIRS += $(DATABASE)/V3IOC
INC += syncChannelFind.h
DBD += PVAServerRegister.dbd
DBD += PVAClientRegister.dbd
LIBSRCS += PVAServerRegister.cpp
LIBSRCS += PVAClientRegister.cpp

SRC_DIRS += $(DATABASE)/special
INC += recordList.h
INC += traceRecord.h
INC += powerSupplyRecordTest.h
LIBSRCS += recordList.cpp
LIBSRCS += traceRecord.cpp

include $(TOP)/configure/RULES

