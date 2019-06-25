TOP = ../..
include $(TOP)/configure/CONFIG

LIBRARY += pvCopy

SHRLIB_VERSION ?= $(EPICS_PVA_MAJOR_VERSION).$(EPICS_PVA_MINOR_VERSION).$(EPICS_PVA_MAINTENANCE_VERSION)

# needed for Windows
LIB_SYS_LIBS_WIN32 += netapi32 ws2_32

INC += pv/pvPlugin.h
INC += pv/pvStructureCopy.h
INC += pv/pvArrayPlugin.h
INC += pv/pvDeadbandPlugin.h
INC += pv/pvTimestampPlugin.h

LIBSRCS += pvPlugin.cpp
LIBSRCS += pvCopy.cpp
LIBSRCS += pvArrayPlugin.cpp
LIBSRCS += pvDeadbandPlugin.cpp
LIBSRCS += pvTimestampPlugin.cpp

pvCopy_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
pvCopy_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

