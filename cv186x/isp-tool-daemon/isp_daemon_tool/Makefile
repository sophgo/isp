SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

include Makefile.releaseList
include Makefile.cvirtsp
include ../../../isp.mk

MW_SAMPLE_COMMON_PATH=$(TOP_DIR)/middleware/$(MW_VER)/sample/common
include $(BUILD_PATH)/.config
include $(MW_SAMPLE_COMMON_PATH)/../Kbuild

# STREAM_TYPE 2:live555 3:normal output to panel
STREAM_TYPE ?= 2

# For STREAM_TYPE=3
# VPSS Out
VPSS_OUT_WIDTH ?= 1280
VPSS_OUT_HEIGHT ?= 720
SDIR = $(PWD)/src
TMP_FOLDER = tmp
ISP_COMMON_DIR = $(PWD)/../../../common

ifeq ($(DESTDIR),)
	DESTDIR := $(shell pwd)/install
endif

INCS = -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -I$(VI_INC) -Iinclude -I$(MW_PATH)/sample -I$(CVI_ISPD2_PATH)
INCS += -I$(ISP_COMMON_DIR)/raw_dump/inc
INCS += -I$(ISP_COMMON_DIR)/raw_dump/src

OBJS = $(patsubst $(SDIR)/%.cpp, $(TMP_FOLDER)/%.o, $(wildcard $(SDIR)/*.cpp))
COBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(wildcard $(SDIR)/*.c))
DEPS = $(patsubst $(SDIR)/%.cpp, $(TMP_FOLDER)/%.d, $(wildcard $(SDIR)/*.cpp))
CDEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(wildcard $(SDIR)/*.c))
AI_SDK_PATH = $(OUTPUT_DIR)/tpu_$(SDK_VER)/cvitek_ai_sdk
TPU_SDK_PATH = $(TOP_DIR)/libsophon/install/libsophon-0.4.9/lib

# mw sample common
SAMPLE_SRCS = $(wildcard $(MW_SAMPLE_COMMON_PATH)/*.c)
SAMPLE_OBJS = $(SAMPLE_SRCS:.c=.o)
SAMPLE_DEPS = $(SAMPLE_SRCS:.c=.d)
PANEL_INC = $(TOP_DIR)/middleware/$(MW_VER)/component/panel/$(shell echo $(CVIARCH) | tr A-Z a-z)
MW_SAMPLE_CFLAGS = -I$(MW_SAMPLE_COMMON_PATH) -I$(MW_INC) -I$(ISP_INC) -I$(VI_INC) -I$(KERNEL_INC) -I$(PANEL_INC)
MW_SAMPLE_CFLAGS += -MMD -MP
MW_SAMPLE_CFLAGS += $(KBUILD_DEFINES)

ifeq ($(SDK_VER), 32bit)
	OPENCV_SDK_LIB_PATH = $(TOP_DIR)/middleware/$(MW_VER)/3rdparty/opencv/lib_32
else ifeq ($(SDK_VER), 64bit)
	OPENCV_SDK_LIB_PATH = $(TOP_DIR)/middleware/$(MW_VER)/3rdparty/opencv/lib_64
else
	OPENCV_SDK_LIB_PATH =
endif

CTRL_SRC_DIR = $(PWD)/ctrl_src
CTRL_OBJS = $(patsubst $(CTRL_SRC_DIR)/%.cpp, $(TMP_FOLDER)/%.o, $(wildcard $(CTRL_SRC_DIR)/*.cpp))
CTRL_DEPS = $(patsubst $(CTRL_SRC_DIR)/%.cpp, $(TMP_FOLDER)/%.d, $(wildcard $(CTRL_SRC_DIR)/*.cpp))

AI_SDK_PATH_EXIST = $(shell if [ -d $(AI_SDK_PATH) ]; then echo "exist"; else echo "noexist"; fi)
TPU_SDK_PATH_EXIST = $(shell if [ -d $(TPU_SDK_PATH) ]; then echo "exist"; else echo "noexist"; fi)

TARGET = isp_tool_daemon
CTRL_TARGET = isp_tool_daemon_ctrl
OUT_TARBALL = isp_tool_daemon.tar.gz

PKG_CONFIG_PATH = $(MW_PATH)/pkgconfig
REQUIRES = cvi_common cvi_sample cvi_vdec cvi_misc
MW_LIBS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --define-variable=mw_dir=$(MW_PATH) $(REQUIRES))

LIBS += $(MW_LIBS) -latomic
LIBS += -lcvi_bin -lcvi_bin_isp -lsns_full -lini
LIBS += -ljson-c -lcvi_ispd2
LIBS += -lraw_dump
LIBS += -lteaisp -lbmlib -lbmrt

LOCAL_CFLAGS = $(DEFS) $(INCS) -DSDK_VER=$(SDK_VER)
LOCAL_CFLAGS += -MMD -MP
LOCAL_CPPFLAGS = -fno-use-cxa-atexit
ifeq ($(SDK_VER), uclibc)
LOCAL_LDFLAGS = $(LIBS) -Wl,-rpath-link=$(CVI_RTSP_LIB_PATH) -L$(CVI_ISPD2_PATH) -L$(CVI_RTSP_LIB_PATH) -lpthread
else
LOCAL_LDFLAGS = $(LIBS) -Wl,-rpath-link=$(CVI_RTSP_LIB_PATH) -L$(CVI_ISPD2_PATH) -L$(CVI_RTSP_LIB_PATH) -lm -lpthread
endif

LOCAL_LDFLAGS += -L$(TPU_SDK_PATH)

ifeq ($(STREAM_TYPE), 2)
INCS += -I$(CVI_RTSP_INC) -I$(CVI_RTSP_SERVICE_INC)
LIBS += -L/source/cvi_rtsp/src -L/source/cvi_rtsp/service -ldl -lcvi_rtsp -lcvi_rtsp_service
DEFS += -DSTREAM_TYPE=2
else
LIBS += -ldl
DEFS += -DVPSS_OUT_WIDTH=$(VPSS_OUT_WIDTH) -DVPSS_OUT_HEIGHT=$(VPSS_OUT_HEIGHT)
DEFS += -DSTREAM_TYPE=3
endif

.PHONY: clean all package install uninstall sdk_release test
ifeq ($(ISP_SRC_RELEASE),1)
all: sdk_release
else
all: package
endif

prepare: | $(TMP_FOLDER)
ifeq ($(STREAM_TYPE), 2)
ifneq "$(wildcard $(CVI_RTSP_TARBALL_PATH))" ""
	@tar -zxf $(CVI_RTSP_TARBALL_PATH) -C $(TMP_FOLDER)
else
	$(error Can't find $(CVI_RTSP_PATH))
endif
endif

$(TMP_FOLDER)/%.o: $(SDIR)/%.cpp | prepare $(TMP_FOLDER)
	$(CXX) $(CXXFLAGS) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -c $< -o $@
	@echo [$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | prepare $(TMP_FOLDER)
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(MW_SAMPLE_COMMON_PATH)/%.o: $(MW_SAMPLE_COMMON_PATH)/%.c
	$(CC) $(CFLAGS) $(MW_SAMPLE_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(OBJS) $(COBJS) $(SAMPLE_OBJS)
	$(CXX) -o $@ $(OBJS) $(COBJS) $(SAMPLE_OBJS) $(ELFFLAGS) $(LOCAL_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER)/%.o: $(CTRL_SRC_DIR)/%.cpp | prepare $(TMP_FOLDER)
	$(CXX) $(CXXFLAGS) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -c $< -o $@
	@echo [$(notdir $(CXX))] $(notdir $@)

$(CTRL_TARGET): $(CTRL_OBJS) $(SAMPLE_OBJS)
	$(CXX) -o $@ $(CTRL_OBJS) $(SAMPLE_OBJS) $(ELFFLAGS) $(LOCAL_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER):
	-@mkdir -p $(TMP_FOLDER)

package: $(TARGET) $(CTRL_TARGET)
	@rm -rf install/*
	@mkdir -p install/lib
	@cp $(TARGET) install/
	@cp ../CviIspTool.sh install/
ifeq ($(ENABLE_ISP_IPC),1)
	@cp $(CTRL_TARGET) install/
	@cp ../CviIspCtrlTool.sh install/
endif
	@cp ../config.ini install/
	@cp ../cfg.json install/
	@cp ../vc_param.json install/

	@mkdir install/lib/ai
ifeq "$(TPU_SDK_PATH_EXIST)" "noexist"
	$(info libsophon install/lib not exist)
else
	@cp $(TPU_SDK_PATH)/*.so* install/lib/ai -rf
endif

ifeq "$(AI_SDK_PATH_EXIST)" "noexist"
	$(info cvitek_ai_sdk not exist, please export TPU_REL=1 run build_all !!)
else
	@cp $(AI_SDK_PATH)/lib/*.so* install/lib/ai -rf
endif

ifeq ($(OPENCV_SDK_LIB_PATH), )
	@echo "No suitable version opencv lib for " $(SDK_VER)
else
	@cp $(OPENCV_SDK_LIB_PATH)/*.so* install/lib/ai -rf
endif

	@for so in $(file); \
	do \
		cp -Lrf $$so install/lib > /dev/null 2>&1; \
	done

ifeq ($(STREAM_TYPE), 2)
	@echo "Pack with live555 related files ..."
	@cp -arf $(CVI_RTSP_OUT_PATH)/* install

	@for data in $(rtsp_lib_nonused); \
	do \
		rm -rf install/$$data; \
	done
endif
	@sed -i 's/isEnableSetSnsCfgPath = 0/isEnableSetSnsCfgPath = 1/g' install/config.ini
	@tar -zcf ../$(OUT_TARBALL) install
	@sed -i 's/isEnableSetSnsCfgPath = 1/isEnableSetSnsCfgPath = 0/g' install/config.ini

install:
	@mkdir -p $(DESTDIR)/usr/bin
	@mkdir -p $(DESTDIR)/usr/lib
	@cp -f install/$(TARGET) $(DESTDIR)/usr/bin
	@cp -f install/CviIspTool.sh $(DESTDIR)/usr/bin
ifeq ($(ENABLE_ISP_IPC),1)
	@cp -f install/$(CTRL_TARGET) $(DESTDIR)/usr/bin
	@cp -f install/CviIspCtrlTool.sh $(DESTDIR)/usr/bin
endif
	@cp -f install/config.ini $(DESTDIR)/usr/bin
	@cp -f install/cfg.json $(DESTDIR)/usr/bin
	@cp -f install/vc_param.json $(DESTDIR)/usr/bin

	-@cp -f install/pqtool_definition.json $(DESTDIR)/usr/bin


uninstall:
	-@rm -f $(DESTDIR)/usr/bin/$(TARGET)
	-@rm -f $(DESTDIR)/usr/bin/CviIspTool.sh
ifeq ($(ENABLE_ISP_IPC),1)
	-@rm -f $(DESTDIR)/usr/bin/$(CTRL_TARGET)
	-@rm -f $(DESTDIR)/usr/bin/CviIspCtrlTool.sh
endif
	-@rm -f $(DESTDIR)/usr/bin/config.ini
	-@rm -f $(DESTDIR)/usr/bin/cfg.json
	-@rm -f $(DESTDIR)/usr/bin/vc_param.json
	-@rm -f $(DESTDIR)/usr/bin/pqtool_definition.json

sdk_release: package
	-@rm -rf include src tmp
	-@mv Makefile.sdk_release Makefile
	-@rm -f Makefile.*

	-@rm -f ../cfg.json ../vc_param.json ../config.ini ../CviIspTool.sh ../README.md ../.gitignore
	-@rm -rf ../test

clean:
	@rm -f $(OBJS) $(DEPS) $(COBJS) $(SAMPLE_OBJS) $(CDEPS) $(TARGET) $(CTRL_OBJS) $(CTRL_DEPS) $(CTRL_TARGET) $(SAMPLE_DEPS)
	@rm -rf install $(TMP_FOLDER)
	@rm -rf ../$(OUT_TARBALL)

test:
	@echo "mw sample objectd: " $(SAMPLE_OBJS)
	@echo "mw sample cflags: "  $(MW_SAMPLE_CFLAGS)

-include $(DEPS) $(CDEPS) $(CTRL_DEPS) $(SAMPLE_DEPS)

