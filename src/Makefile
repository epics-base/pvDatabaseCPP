TOP = ..
include $(TOP)/configure/CONFIG

DATABASE = $(TOP)/src/

LIBRARY_IOC += pvDatabase
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)
pvDatabase_LIBS += Com pvData pvAccess

SRC_DIRS += $(DATABASE)/database
INC += pvDatabase.h
INC += powerSupplyRecordTest.h
LIBSRCS += pvRecord.cpp

SRC_DIRS += $(DATABASE)/pvAccess
INC += pvCopy.h
LIBSRCS += pvCopy.cpp

include $(TOP)/configure/RULES

