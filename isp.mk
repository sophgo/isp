ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

isp_chip_dir := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)

ifeq ($(CHIP_ARCH), CV183X)
ENABLE_ISP_IPC = 0
else ifeq ($(CHIP_ARCH), CV182X)
ENABLE_ISP_IPC = 0
else
ENABLE_ISP_IPC ?= 1
endif

ifeq ($(ENABLE_ISP_IPC),1)
	CFLAGS += -DENABLE_ISP_IPC
endif

ifeq ($(V4L2_ISP_ENABLE),1)
	CFLAGS += -DV4L2_ISP_ENABLE
endif
