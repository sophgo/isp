SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

INCS = -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -I$(VI_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC) -Iinc -Isrc -I../isp/inc
INCS += -I../../common/clog

RELEASE_FOLDER = $(CURDIR)/bin
SDIR := $(CURDIR)/src
SRCS := $(SDIR)/dpcm_api.c \
	$(SDIR)/isp_algo_blc.c \
	$(SDIR)/isp_algo_bnr.c \
	$(SDIR)/isp_algo_ca.c \
	$(SDIR)/isp_algo_ca2.c \
	$(SDIR)/isp_algo_cac.c \
	$(SDIR)/isp_algo_ccm.c \
	$(SDIR)/isp_algo_clut.c \
	$(SDIR)/isp_algo_cnr.c \
	$(SDIR)/isp_algo_crosstalk.c \
	$(SDIR)/isp_algo_csc.c \
	$(SDIR)/isp_algo_dci.c \
	$(SDIR)/isp_algo_debug.c \
	$(SDIR)/isp_algo_dehaze.c \
	$(SDIR)/isp_algo_demosaic.c \
	$(SDIR)/isp_algo_dis.c \
	$(SDIR)/isp_algo_dpc.c \
	$(SDIR)/isp_algo_drc.c \
	$(SDIR)/isp_algo_fswdr.c \
	$(SDIR)/isp_algo_gamma.c \
	$(SDIR)/isp_algo_lblc.c \
	$(SDIR)/isp_algo_lcac.c \
	$(SDIR)/isp_algo_ldci.c \
	$(SDIR)/isp_algo_mlsc.c \
	$(SDIR)/isp_algo_mono.c \
	$(SDIR)/isp_algo_motion.c \
	$(SDIR)/isp_algo_presharpen.c \
	$(SDIR)/isp_algo_rgbcac.c \
	$(SDIR)/isp_algo_rgbir.c \
	$(SDIR)/isp_algo_sharpen.c \
	$(SDIR)/isp_algo_teaisp_bnr.c \
	$(SDIR)/isp_algo_teaisp_drc.c \
	$(SDIR)/isp_algo_teaisp_pq.c \
	$(SDIR)/isp_algo_tnr.c \
	$(SDIR)/isp_algo_utility.c \
	$(SDIR)/isp_algo_wb.c \
	$(SDIR)/isp_algo_ycontrast.c \
	$(SDIR)/isp_algo_ynr.c \
	$(SDIR)/isp_iir_api.c \
	$(SDIR)/pchip.c

TARGET_MACHINE := $(shell $(CC) -dumpmachine)
CHIP_ARCH_L := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
OBJECT_TAG := $(TARGET_MACHINE).$(CHIP_ARCH_L)$(if $(filter 1,$(V4L2_ISP_ENABLE)),.v4l2,)
ODIR := $(CURDIR)/obj
OBJS := $(SRCS:$(SDIR)/%.c=$(ODIR)/%.$(OBJECT_TAG).o)
DEPS := $(OBJS:.o=.d)
SOURCE_FILES := $(wildcard $(SRCS))

ifneq ($(filter-out clean clean_all,$(or $(MAKECMDGOALS),all)),)
ifneq ($(strip $(SOURCE_FILES)),)
ifneq ($(words $(SOURCE_FILES)),$(words $(SRCS)))
$(error Incomplete ISP algorithm sources. Restore all sources or use a complete prebuilt object set.)
endif
endif
endif

TARGET_A = $(MW_LIB)/libisp_algo.a
TARGET_SO = $(MW_LIB)/libisp_algo.so

LOCAL_CFLAGS = $(INCS)
LOCAL_LDFLAGS =

# Toolchain
ifeq ($(SDK_VER), glibc_riscv64)
	LOCAL_CFLAGS += -DSDK_ARM_ARCH=0 -DSDK_LIB_BIT=64
else ifeq ($(SDK_VER), musl_riscv64)
	LOCAL_CFLAGS += -DSDK_ARM_ARCH=0 -DSDK_LIB_BIT=64
else ifeq ($(SDK_VER), 32bit)
	LOCAL_CFLAGS += -DSDK_ARM_ARCH=1 -DSDK_LIB_BIT=32
	LOCAL_CFLAGS += -mfpu=neon
else ifeq ($(SDK_VER), uclibc)
	LOCAL_CFLAGS += -DSDK_ARM_ARCH=1 -DSDK_LIB_BIT=32
	LOCAL_CFLAGS += -DIGNORE_LOG_WARNING -DIGNORE_LOG_NOTICE -DIGNORE_LOG_INFO -DIGNORE_LOG_DEBUG
	LOCAL_CFLAGS += -mfpu=neon
else
	LOCAL_CFLAGS += -DSDK_ARM_ARCH=1 -DSDK_LIB_BIT=64
endif

.PHONY: clean all clean_all sdk_release

ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(ODIR)/%.$(OBJECT_TAG).o: $(SDIR)/%.c
	@mkdir -p $(dir $@)
	echo LOCAL_CFLAGS_EVAN=$(LOCAL_CFLAGS)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(LOCAL_LDFLAGS) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

sdk_release: $(TARGET_A) $(TARGET_SO)
	@echo "Create $(RELEASE_FOLDER)"
	-@mkdir -p $(RELEASE_FOLDER)
	-@rm -rf $(RELEASE_FOLDER)/*
	-@cp $(TARGET_A) $(RELEASE_FOLDER)
	-@cp $(TARGET_SO) $(RELEASE_FOLDER)
	-@rm -rf src
	-@rm -rf $(ODIR)
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
ifeq ($(words $(SOURCE_FILES)),$(words $(SRCS)))
	@rm -f $(OBJS) $(DEPS)
endif

ifeq ($(words $(SOURCE_FILES)),$(words $(SRCS)))
-include $(DEPS)
endif
