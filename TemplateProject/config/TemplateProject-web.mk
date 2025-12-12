# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/TemplateProject/config
IPLUG2_ROOT = ../../iPlug2

include ../../common-web.mk

SRC += $(PROJECT_ROOT)/TemplateProject.cpp

WAM_SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp

WAM_CFLAGS += -I$(IPLUG_SYNTH_PATH)

# Q DSP Library include paths
Q_LIB_PATH = $(PROJECT_ROOT)/q_lib/q_lib
WAM_CFLAGS += -I$(Q_LIB_PATH)/include -I$(Q_LIB_PATH)/infra/include
WEB_CFLAGS += -I$(Q_LIB_PATH)/include -I$(Q_LIB_PATH)/infra/include

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
