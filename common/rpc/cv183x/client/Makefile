
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

TARGET_A = $(MW_LIB)/libisp_rpc_client.a
TARGET_SO = $(MW_LIB)/libisp_rpc_client.so

isp_chip_name := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
ISP_BASE_PATH = $(PWD)/../../../$(isp_chip_name)

OBJDIR = ./obj

INC_DIR += -I./
INC_DIR += -I../rpcgen
INC_DIR += -I$(ISP_BASE_PATH)/include	# 183x
INC_DIR += -I$(MW_INC)
INC_DIR += -I$(ISP_INC)

CFLAGS += $(INC_DIR)

SRC_DIR += .

VPATH = $(SRC_DIR)

SRC_C = $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c')

OBJ_C_0 = $(notdir $(patsubst %.c, %.o, $(SRC_C)))

OBJ_C    = $(foreach file, $(OBJ_C_0), $(OBJDIR)/$(file))

DEPS_C   = $(OBJ_C:%.o=%.d)

.PHONY: all setup target clean

all: setup target

setup:
	@mkdir -p $(OBJDIR)

target: setup $(TARGET_A) $(TARGET_SO)

$(TARGET_A): $(OBJ_C)
	@$(AR) $(ARFLAGS) $@ $(OBJ_C)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJ_C)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ --start-group $(OBJ_C) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

$(OBJ_C): $(OBJDIR)/%.o : %.c
	@$(CC) $(CFLAGS) -o $@ -c $<

clean:
	@rm -rf $(OBJDIR)
	@rm -rf *.so *.a

$(DEPS_C): $(OBJDIR)/%.d : %.c
	@mkdir -p $(OBJDIR); \
	set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJDIR)\/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS_C)
