SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif
include ../../isp.mk

TMP_FOLDER = $(PWD)/tmp
SDIR = $(PWD)/src
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC) -Iinc -Isrc
INCS += -I../isp/inc
INCS += -I../isp/src
INCS += -I../isp_algo/inc
INCS += -I../../algo
INCS += -I../../common/inc
INCS += -I../../common/clog
INCS += -I../../common/isp_ipc
INCS += -I$(TOP_DIR)/libsophon/bmlib/include
INCS += -I$(TOP_DIR)/libsophon/tpu-runtime/include
TPU_SDK_PATH = $(TOP_DIR)/libsophon/install/libsophon-0.4.9/lib

SRCS = $(wildcard $(SDIR)/*.c)

OBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(SRCS))
DEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(SRCS))

TARGET_A = $(MW_LIB)/libteaisp.a
TARGET_SO = $(MW_LIB)/libteaisp.so

LOCAL_CFLAGS = $(INCS) -std=gnu11
LOCAL_LDFLAGS = -L$(TPU_SDK_PATH)
ifeq ($(V4L2_ISP_ENABLE),1)
LOCAL_LDFLAGS += -L../isp-tool-daemon-v4l2/prebuilt/libarm64
endif
LOCAL_LDFLAGS += -lbmlib -lbmrt

# SOC Architecture
ifeq ($(CHIP_ARCH), CV183X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV183X
else ifeq ($(CHIP_ARCH), CV182X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV182X
else ifeq ($(CHIP_ARCH), CV181X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV181X
else ifeq ($(CHIP_ARCH), CV180X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV180X
else ifeq ($(CHIP_ARCH), CV186X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV186X
else
	$(error Un-support SOC Architecture)
endif

# Toolchain
ifeq ($(SDK_VER), glibc_riscv64)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
else ifeq ($(SDK_VER), musl_riscv64)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
else ifeq ($(SDK_VER), 32bit)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=32
else ifeq ($(SDK_VER), uclibc)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=32
	LOCAL_CFLAGS += -DIGNORE_LOG_ERR -DIGNORE_LOG_WARNING -DIGNORE_LOG_NOTICE -DIGNORE_LOG_INFO -DIGNORE_LOG_DEBUG
	LOCAL_CFLAGS += -DAPI_ATTR_CHECK_LOG_LEVEL=1
else
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
endif

.PHONY: clean all cleanall sdk_release

ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	@echo LOCAL_CFLAGS_EVAN=$(LOCAL_CFLAGS)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(LOCAL_LDFLAGS) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

$(TMP_FOLDER):
	@echo "Create $(TMP_FOLDER) for object file"
	-@mkdir -p $(TMP_FOLDER)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

cleanall: clean
	@rm -rf $(TMP_FOLDER)

sdk_release:
	-@rm -rf $(TMP_FOLDER)
	-@mv Makefile.sdk_release Makefile

-include $(DEPS)
