SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

CVI_RTSP_PATH := $(MW_PATH)/../../cvi_rtsp
CVI_RTSP_INC := $(CVI_RTSP_PATH)/include
#CVI_RTSP_LIB_PATH := $(CVI_RTSP_OUT_PATH)/lib
TPU_SDK_PATH = $(TOP_DIR)/libsophon

TMP_FOLDER = $(PWD)/tmp
SDIR = $(PWD)/src

SRCS = $(wildcard $(SDIR)/*.c)
SRCS := $(filter-out $(SDIR)/self_test.c, $(SRCS))

INCS += -I./inc
INCS += -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I$(VI_INC)
INCS += -I../../common/clog
INCS += -I../isp/inc
INCS += -I../v4l2_adapter/inc
INCS += -I../isp-daemon2/inc
INCS += -I$(CVI_RTSP_INC)
INCS += -I$(MW_INC)/3rd/drm
INCS += -I$(MW_INC)/3rd/drm/libdrm
# json-c
INCS += -I$(OUTPUT_DIR)/mw_$(SDK_VER)/cvitek_mw_sdk/include

INCS += -I$(TPU_SDK_PATH)/bmlib/include
INCS += -I$(TPU_SDK_PATH)/libsophav/bmcv/include

OBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(SRCS))
DEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(SRCS))

TARGET = isp_tool_daemon

EXTRA_CFLAGS = $(INCS)
CFLAGS += -MMD -MP -DV4L2_ISP_ENABLE

ELFFLAGS += -L$(MW_PATH)/lib
ELFFLAGS += -L$(MW_PATH)/lib/3rd/drm

MW_LIBS := -lispv4l2_adapter -lcvi_bin -lcvi_bin_isp -lsns_full -ljson-c -ldrm
ISP_LIBS := -lisp -lawb -lae -laf -lisp_algo -lcvi_ispd2 -lvenc -lcvi_rtsp
ISP_HELPER_LIBS := -lispv4l2_helper
TPU_LIBS := -lbmcv -lbmlib -lbmjpeg
LIBS += $(MW_LIBS) $(ISP_LIBS) $(ISP_HELPER_LIBS) $(TPU_LIBS) -L./prebuilt/libarm64
ELFFLAGS += $(LIBS) -lpthread -lm -latomic -ldl

.PHONY : clean all self_test

all: $(TARGET)

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(OBJS)
	$(CXX) -o $@ -Wl,--start-group $(OBJS) -Wl,--end-group $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER):
	@echo "Create $(TMP_FOLDER) for object file"
	-@mkdir -p $(TMP_FOLDER)

clean:
	-@rm -f $(OBJS) $(DEPS) $(TARGET)
	-@rm -rf $(TMP_FOLDER)

self_test:
	@$(CC) $(INCS) $(SDIR)/self_test.c $(SDIR)/utils.c -o self_test $(ELFFLAGS)

-include $(DEPS)
