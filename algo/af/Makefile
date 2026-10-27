SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

SDIR := $(CURDIR)
SRCS := $(SDIR)/af_buf.c \
	$(SDIR)/af_calib.c \
	$(SDIR)/afalgo.c \
	$(SDIR)/cvi_af.c
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC) -I../

ifneq ($(findstring $(CHIP_ARCH), CV183X cv183x),)
CFLAGS += -DCHIP_ARCH_CV183X
INCS += -I../../cv183x/include
else ifneq ($(findstring $(CHIP_ARCH), CV182X cv182x),)
CFLAGS += -DCHIP_ARCH_CV182X
INCS += -I../../cv182x/isp/inc
else ifneq ($(findstring $(CHIP_ARCH), CV181X cv181x),)
CFLAGS += -DCHIP_ARCH_CV181X
INCS += -I../../cv181x/isp/inc
else ifneq ($(findstring $(CHIP_ARCH), CV180X cv180x),)
CFLAGS += -DCHIP_ARCH_CV180X
INCS += -I../../cv180x/isp/inc
else ifneq ($(findstring $(CHIP_ARCH), CV186X cv186x),)
CFLAGS += -DCHIP_ARCH_CV186X
INCS += -I../../cv186x/isp/inc
INCS += -I../ae
endif
INCS += -I../../common/clog

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
TARGET_A = $(MW_LIB)/libaf.a
TARGET_SO = $(MW_LIB)/libaf.so

EXTRA_CFLAGS = $(INCS)

CFLAGS += -Wno-format-truncation


.PHONY: all clean clean_all sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(ODIR)/%.$(OBJECT_TAG).o: $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
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
	-@rm -f *.c *.h
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
