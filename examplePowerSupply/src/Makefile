TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#


PROD_HOST += powerSupplyMain
powerSupplyMain_SRCS += powerSupplyMain.cpp
powerSupplyMain_LIBS += Com
powerSupplyMain_LIBS += pvData
powerSupplyMain_LIBS += pvAccess
powerSupplyMain_LIBS += pvDatabase
powerSupplyMain_LIBS += powerSupply

DBD += powerSupply.dbd

INC += powerSupply.h

LIBRARY_IOC += powerSupply
powerSupply_SRCS += powerSupply.cpp
powerSupply_SRCS += powerSupplyRegister.cpp
powerSupply_LIBS += pvData
powerSupply_LIBS += pvAccess
powerSupply_LIBS += pvDatabase
powerSupply_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

