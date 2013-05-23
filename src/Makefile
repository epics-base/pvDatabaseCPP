TOP = ..
include $(TOP)/configure/CONFIG

DATABASE = $(TOP)/src/

LIBRARY_IOC += pvDatabase
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)
pvDatabase_LIBS += Com pvData pvAccess

SRC_DIRS += $(DATABASE)/database
INC += pvDatabase.h
INC += powerSupplyRecordTest.h
INC += exampleCounter.h
INC += recordList.h
LIBSRCS += pvRecord.cpp
LIBSRCS += pvDatabase.cpp
LIBSRCS += recordList.cpp

SRC_DIRS += $(DATABASE)/pvAccess
INC += channelProviderLocal.h
INC += pvCopy.h
INC += monitorAlgorithm.h
INC += channelLocalDebugRecord.h
LIBSRCS += channelProviderLocal.cpp
LIBSRCS += channelLocal.cpp
LIBSRCS += pvCopy.cpp
LIBSRCS += monitorFactory.cpp
LIBSRCS += channelLocalDebugRecord.cpp


include $(TOP)/configure/RULES

