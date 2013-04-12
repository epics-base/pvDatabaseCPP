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
LIBSRCS += pvRecord.cpp
LIBSRCS += pvDatabase.cpp

SRC_DIRS += $(DATABASE)/pvAccess
INC += channelProviderLocal.h
INC += pvCopy.h
LIBSRCS += channelProviderLocal.cpp
LIBSRCS += channelLocal.cpp
LIBSRCS += pvCopy.cpp

include $(TOP)/configure/RULES

