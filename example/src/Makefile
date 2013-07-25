TOP = ../..
include $(TOP)/configure/CONFIG

EXAMPLESRC = $(TOP)/example/src/

LIBRARY_IOC += pvDatabaseExample
pvDatabaseExample_LIBS += $(EPICS_BASE_IOC_LIBS)
pvDatabaseExample_LIBS += Com pvData pvAccess pvDatabase

SRC_DIRS += $(EXAMPLESRC)/exampleCounter
INC += exampleCounter.h

SRC_DIRS += $(EXAMPLESRC)/exampleServer
INC+= exampleServerCreateRecords.h
LIBSRCS += exampleServerCreateRecords.cpp


include $(TOP)/configure/RULES

