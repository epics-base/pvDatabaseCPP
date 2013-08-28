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

SRC_DIRS += $(EXAMPLESRC)/examplePVADoubleArrayGet
INC+= examplePVADoubleArrayGet.h
LIBSRCS += examplePVADoubleArrayGet.cpp

SRC_DIRS += $(EXAMPLESRC)/arrayPerformance
INC+= arrayPerformance.h
LIBSRCS += arrayPerformance.cpp
INC+= longArrayMonitor.h
LIBSRCS += longArrayMonitor.cpp


include $(TOP)/configure/RULES

