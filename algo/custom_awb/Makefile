SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

SDIR = $(PWD)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I../

ifneq ($(findstring $(CHIP_ARCH), CV183X cv183x),)
CFLAGS += -DCHIP_ARCH_CV183X
else ifneq ($(findstring $(CHIP_ARCH), CV182X cv182x),)
CFLAGS += -DCHIP_ARCH_CV182X
endif

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libawb.a
TARGET_SO = $(MW_LIB)/libawb.so

EXTRA_CFLAGS = $(INCS)


.PHONY: clean all
all: $(TARGET_A) $(TARGET_SO)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

-include $(DEPS)
