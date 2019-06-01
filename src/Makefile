# Makefile for the pvDatabase library

TOP = ..
include $(TOP)/configure/CONFIG

PVDATABASE_SRC = $(TOP)/src

LIBRARY += pvDatabase

# shared library ABI version.
SHRLIB_VERSION ?= 4.4.2

INC += pv/channelProviderLocal.h
INC += pv/pvDatabase.h
INC += pv/traceRecord.h
INC += pv/removeRecord.h
INC += pv/numericRecord.h
INC += pv/pvSupport.h
INC += pv/controlSupport.h
include $(PVDATABASE_SRC)/copy/Makefile
include $(PVDATABASE_SRC)/database/Makefile
include $(PVDATABASE_SRC)/pvAccess/Makefile
include $(PVDATABASE_SRC)/special/Makefile
include $(PVDATABASE_SRC)/support/Makefile

pvDatabase_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
