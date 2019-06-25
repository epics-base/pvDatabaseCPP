#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS += configure

DIRS += src/copy
src/copy_DEPEND_DIRS = configure


DIRS += src
src_DEPEND_DIRS = configure src/copy

DIRS += test
test_DEPEND_DIRS = src

include $(TOP)/configure/RULES_TOP
