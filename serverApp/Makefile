TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USR_CXXFLAGS += -pthread -fomit-frame-pointer

PROD = testServer
SRCS += serverMain.cpp

USR_CPPFLAGS += -std=c++11 -ggdb
USR_SYS_LIBS += dl pthread xml2 boost_system 
USR_SYS_LIBS += opcuaserver opcuaprotocol opcuacore
USR_LDFLAGS  += -L$(FREEOPCUA)/.libs

USR_INCLUDES += -I$(FREEOPCUA)/include
USR_INCLUDES += -I/usr/include/libxml2
USR_LDFLAGS += '-Wl,-rpath,$(FREEOPCUA)/.libs'

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
