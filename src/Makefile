TOP = ..
include $(TOP)/configure/CONFIG

DATABASE = $(TOP)/src/

LIBRARY_IOC += pvDatabase
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)
pvDatabase_LIBS += Com pvData pvAccess

SRC_DIRS += $(DATABASE)/database
INC += pvDatabase.h
#INC += pvCopy.h
#INC += monitor.h
LIBSRCS += pvDatabase.cpp
#LIBSRCS += pvRecord.cpp
#LIBSRCS += recordProcess.cpp
#LIBSRCS += pvCopy.cpp
#LIBSRCS += monitor.cpp

#SRC_DIRS += $(SRC)/pvAccess

include $(TOP)/configure/RULES

