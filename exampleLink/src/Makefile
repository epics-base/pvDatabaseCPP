TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBD += exampleLink.dbd

INC += exampleLink.h

LIBRARY_IOC += exampleLinkSupport
exampleLinkSupport_SRCS += exampleLink.cpp
exampleLinkSupport_SRCS += exampleLinkRegister.cpp
exampleLinkSupport_LIBS += pvData
exampleLinkSupport_LIBS += pvAccess
exampleLinkSupport_LIBS += pvDatabase
exampleLinkSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

