# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/EffectTemplate/config
IPLUG2_ROOT = ../../iPlug2

include ../../common-web.mk

SRC += $(PROJECT_ROOT)/EffectTemplate.cpp

# Q DSP Library paths
Q_LIB_PATH = $(PROJECT_ROOT)/q_lib/q_lib
WAM_CFLAGS += -I$(Q_LIB_PATH)/include -I$(Q_LIB_PATH)/infra/include
WEB_CFLAGS += -I$(Q_LIB_PATH)/include -I$(Q_LIB_PATH)/infra/include

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
