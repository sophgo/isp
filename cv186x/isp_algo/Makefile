SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

INCS = -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -I$(VI_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC) -Iinc -Isrc -I../isp/inc
INCS += -I../../common/clog

TMP_FOLDER = $(PWD)/tmp
RELEASE_FOLDER = $(PWD)/bin
SDIR = $(PWD)/src
SRCS = $(shell find $(SDIR) -type f -name "*.c")

OBJS = $(addsuffix .o,$(basename $(addprefix $(TMP_FOLDER)/,$(notdir $(SRCS)))))
DEPS = $(addsuffix .d,$(basename $(addprefix $(TMP_FOLDER)/,$(notdir $(SRCS)))))

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

.PHONY: clean all cleanall sdk_release

ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	echo LOCAL_CFLAGS_EVAN=$(LOCAL_CFLAGS)
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

sdk_release: $(TARGET_A) $(TARGET_SO)
	@echo "Create $(RELEASE_FOLDER)"
	-@mkdir -p $(RELEASE_FOLDER)
	-@rm -rf $(RELEASE_FOLDER)/*
	-@cp $(TARGET_A) $(RELEASE_FOLDER)
	-@cp $(TARGET_SO) $(RELEASE_FOLDER)
	-@rm -rf src
	-@rm -rf $(TMP_FOLDER)
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

cleanall: clean
	@rm -rf $(TMP_FOLDER)
	@rm -rf $(RELEASE_FOLDER)

-include $(DEPS)
