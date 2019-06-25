# Makefile for the pvDatabase library

TOP = ..
include $(TOP)/configure/CONFIG

# needed for Windows
LIB_SYS_LIBS_WIN32 += netapi32 ws2_32

PVDATABASE_SRC = $(TOP)/src

LIBRARY += pvDatabase

# shared library ABI version.
SHRLIB_VERSION ?= $(EPICS_PVA_MAJOR_VERSION).$(EPICS_PVA_MINOR_VERSION).$(EPICS_PVA_MAINTENANCE_VERSION)

INC += pv/channelProviderLocal.h
INC += pv/pvDatabase.h
INC += pv/traceRecord.h
INC += pv/removeRecord.h
INC += pv/processRecord.h
INC += pv/pvSupport.h
INC += pv/controlSupport.h
INC += pv/scalarAlarmSupport.h

include $(PVDATABASE_SRC)/database/Makefile
include $(PVDATABASE_SRC)/pvAccess/Makefile
include $(PVDATABASE_SRC)/special/Makefile
include $(PVDATABASE_SRC)/support/Makefile

pvDatabase_LIBS += pvCopy
pvDatabase_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
