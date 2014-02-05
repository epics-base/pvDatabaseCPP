TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBD += exampleServer.dbd

INC += exampleServer.h

LIBRARY_IOC += exampleServerSupport
exampleServerSupport_SRCS += exampleServer.cpp
exampleServerSupport_SRCS += exampleServerRegister.cpp
exampleServerSupport_LIBS += pvData
exampleServerSupport_LIBS += pvAccess
exampleServerSupport_LIBS += pvDatabase
exampleServerSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

