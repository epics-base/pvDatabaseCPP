# Makefile for the EPICS V4 pvDatabase module

TOP = .
include $(TOP)/configure/CONFIG

DIRS := configure

DIRS += src
src_DEPEND_DIRS = configure

EMBEDDED_TOPS := test
EMBEDDED_TOPS += arrayPerformance
EMBEDDED_TOPS += $(wildcard example*)

DIRS += $(EMBEDDED_TOPS)

define dir_DEP
 $(1)_DEPEND_DIRS = src
endef

$(foreach dir, $(EMBEDDED_TOPS), $(eval $(call dir_DEP,$(dir))))

exampleDatabase_DEPEND_DIRS += test
examplePowerSupply_DEPEND_DIRS += test

include $(TOP)/configure/RULES_TOP
