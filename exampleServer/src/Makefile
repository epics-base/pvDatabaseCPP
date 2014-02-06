TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#


PROD_HOST += exampleServerMain
exampleServerMain_SRCS += exampleServerMain.cpp
exampleServerMain_LIBS += Com
exampleServerMain_LIBS += pvData
exampleServerMain_LIBS += pvAccess
exampleServerMain_LIBS += pvDatabase
exampleServerMain_LIBS += exampleServer

DBD += exampleServer.dbd

INC += exampleServer.h

LIBRARY_IOC += exampleServer
exampleServer_SRCS += exampleServer.cpp
exampleServer_SRCS += exampleServerRegister.cpp
exampleServer_LIBS += pvData
exampleServer_LIBS += pvAccess
exampleServer_LIBS += pvDatabase
exampleServer_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

