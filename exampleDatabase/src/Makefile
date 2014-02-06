TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#


PROD_HOST += exampleDatabaseMain
exampleDatabaseMain_SRCS += exampleDatabaseMain.cpp
exampleDatabaseMain_LIBS += Com
exampleDatabaseMain_LIBS += pvData
exampleDatabaseMain_LIBS += pvAccess
exampleDatabaseMain_LIBS += pvDatabase
exampleDatabaseMain_LIBS += exampleDatabase

DBD += exampleDatabase.dbd

INC += exampleDatabase.h

LIBRARY_IOC += exampleDatabase
exampleDatabase_SRCS += exampleDatabase.cpp
exampleDatabase_SRCS += exampleDatabaseRegister.cpp
exampleDatabase_LIBS += pvData
exampleDatabase_LIBS += pvAccess
exampleDatabase_LIBS += pvDatabase
exampleDatabase_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

