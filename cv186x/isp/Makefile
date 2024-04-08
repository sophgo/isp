SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif
include ../../isp.mk

TMP_FOLDER = $(PWD)/tmp
SDIR = $(PWD)/src
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC) -Iinc -Isrc
INCS += -I../isp_algo/inc
INCS += -I../teaisp/src
INCS += -I../../algo
INCS += -I../../common/inc
INCS += -I../../common/clog
INCS += -I../../common/isp_ipc

SRCS = $(wildcard $(SDIR)/*.c)
SRCS += $(wildcard $(SDIR)/../../../common/clog/*.c)
SRCS += $(wildcard $(SDIR)/../../../common/isp_ipc/*.c)

OBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(SRCS))
DEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(SRCS))

TARGET_A = $(MW_LIB)/libisp.a
TARGET_SO = $(MW_LIB)/libisp.so

LOCAL_CFLAGS = $(INCS) -std=gnu11
LOCAL_LDFLAGS =

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

#LOCAL_CFLAGS += -DFREEZE_FRAME_CO_RELEASE_MODE

.PHONY: clean all cleanall sdk_release
.SILENT: check_kernel_dependence update_isp_version cppcheck

ifeq ($(ISP_SRC_RELEASE),1)
all: update_isp_version $(TARGET_A) $(TARGET_SO) sdk_release
else
all: update_isp_version $(TARGET_A) $(TARGET_SO)
endif

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	@echo LOCAL_CFLAGS_EVAN=$(LOCAL_CFLAGS)
	@echo CFLAGS=$(CFLAGS)
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

update_isp_version:
	echo "#define ISP_VERSION \"T(sophon), V(sophon)\"" > $(PWD)/inc/isp_version.h

sdk_release: update_isp_version
	-@rm -rf $(TMP_FOLDER)
	-@mv Makefile.sdk_release Makefile

cppcheck:
	@cppcheck --enable=all --quiet --template='{file},{line},{severity},{id}:{message}' -I ../../include/$(shell echo $(MW_VER)) -I ./inc -I ./src algo/ src/ 2>cppcheck_output.txt
	@python3 $(MW_PATH)/tool/cppcheckparser/cppcheckparser.py --input cppcheck_output.txt --output cppcheck_output.xlsx

-include $(DEPS)
