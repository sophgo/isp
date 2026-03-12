SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif
include ../../isp.mk

SDIR = $(PWD)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC)

ifneq ($(findstring $(CHIP_ARCH), CV183X cv183x),)
CFLAGS += -DCHIP_ARCH_CV183X
INCS += -I../../cv183x/include
else ifneq ($(findstring $(CHIP_ARCH), CV182X cv182x),)
CFLAGS += -DCHIP_ARCH_CV182X
INCS += -I../../cv182x/isp/inc
else ifneq ($(findstring $(CHIP_ARCH), CV181X cv181x),)
CFLAGS += -DCHIP_ARCH_CV181X
INCS += -I../../cv181x/isp/inc
INCS += -I../../cv181x/isp/src
INCS += -I../../cv181x/isp_algo/inc
else ifneq ($(findstring $(CHIP_ARCH), CV180X cv180x),)
CFLAGS += -DCHIP_ARCH_CV180X
INCS += -I../../cv180x/isp/inc
INCS += -I../../cv180x/isp/src
INCS += -I../../cv180x/isp_algo/inc
else ifneq ($(findstring $(CHIP_ARCH), CV186X cv186x),)
CFLAGS += -DCHIP_ARCH_CV186X
INCS += -I../../cv186x/isp/inc
INCS += -I../../cv186x/isp/src
INCS += -I../../cv186x/isp_algo/inc
endif
INCS += -I../../common/clog
INCS += -I../../common/isp_ipc

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libae.a
TARGET_SO = $(MW_LIB)/libae.so

EXTRA_CFLAGS = $(INCS)

CFLAGS += -Wno-format-truncation


.PHONY: all clean sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

sdk_release: $(TARGET_A) $(TARGET_SO)
	-@rm -f $(OBJS) $(DEPS)
	-@cp $(TARGET_A) .
	-@cp $(TARGET_SO) .
	-@find . -name "*.h" -not -name "ae_debug.h" -exec rm -f {} \;
	-@rm -f *.c
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

-include $(DEPS)
