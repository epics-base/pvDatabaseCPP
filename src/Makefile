TOP = ..
include $(TOP)/configure/CONFIG

DATABASE = $(TOP)/src/

LIBRARY_IOC += pvDatabase
pvDatabase_LIBS += pvAccess pvData Com
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)

SRC_DIRS += $(DATABASE)/database
INC += pvDatabase.h
LIBSRCS += pvRecord.cpp
LIBSRCS += pvDatabase.cpp

SRC_DIRS += $(DATABASE)/pvAccess
INC += channelProviderLocal.h
INC += pvCopyMonitor.h
DBD += registerChannelProviderLocal.dbd
LIBSRCS += channelProviderLocal.cpp
LIBSRCS += channelLocal.cpp
LIBSRCS += pvCopyMonitor.cpp
LIBSRCS += monitorFactory.cpp
LIBSRCS += registerChannelProviderLocal.cpp

SRC_DIRS += $(DATABASE)/special
INC += recordList.h
INC += traceRecord.h
LIBSRCS += recordList.cpp
LIBSRCS += traceRecord.cpp

include $(TOP)/configure/RULES

