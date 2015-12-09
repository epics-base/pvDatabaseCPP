TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#


DBD += exampleRPC.dbd

INC += exampleRPC.h

LIBRARY_IOC += exampleRPC
exampleRPC_SRCS += exampleRPC.cpp
exampleRPC_SRCS += exampleRPCRegister.cpp
exampleRPC_LIBS += pvDatabase
exampleRPC_LIBS += pvAccess
exampleRPC_LIBS += pvData
exampleRPC_LIBS += Com
exampleRPC_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_HOST += exampleRPCMain
exampleRPCMain_SRCS += exampleRPCMain.cpp
exampleRPCMain_LIBS += exampleRPC
exampleRPCMain_LIBS += pvDatabase
exampleRPCMain_LIBS += pvAccess
exampleRPCMain_LIBS += pvData
exampleRPCMain_LIBS += Com

PROD_HOST += move
move_SRCS += positionClient.cpp
move_LIBS += pvAccess pvData Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

