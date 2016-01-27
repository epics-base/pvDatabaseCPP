# Makefile for the pvDatabase library

TOP = ..
include $(TOP)/configure/CONFIG

INSTALL_INCLUDE = $(INSTALL_LOCATION)/include/pv
USR_INCLUDES += -I$(INSTALL_LOCATION)/include

PVDATABASE_SRC = $(TOP)/src

include $(PVDATABASE_SRC)/pv/Makefile
include $(PVDATABASE_SRC)/database/Makefile
include $(PVDATABASE_SRC)/pvAccess/Makefile
include $(PVDATABASE_SRC)/special/Makefile

LIBRARY += pvDatabase

pvDatabase_LIBS += pvAccess pvData Com
pvDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)

# shared library ABI version.
SHRLIB_VERSION ?= 4.2


include $(TOP)/configure/RULES

