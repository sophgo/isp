
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../../../Makefile.param
	include $(PARAM_FILE)
endif

TARGET = rpc_test

OBJDIR = ./obj

INC_DIR += -I./
INC_DIR += -I$(MW_INC)
INC_DIR += -I$(ISP_INC)
INC_DIR += -I../../../../isp-daemon/include

CFLAGS += $(INC_DIR)


LD_OPTS += -lisp_rpc_client
LD_OPTS += -lcvi_ispd -ldl -lz
LD_OPTS += -lboost_system
LD_OPTS += -lsys
LD_OPTS += -lvpu
LD_OPTS += -ljson-c
LD_OPTS += -lcvi_bin
LD_OPTS += -lisp_bin
LD_OPTS += -lcvitracer
LD_OPTS += -lnanomsg
LD_OPTS += -lvenc -lvdec -lcvi_jpeg -lcvi_vcodec
LD_OPTS += -L$(MW_LIB)
LD_OPTS += -L$(MW_LIB)/3rd

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

target: setup $(OBJ_C)
	@$(CC) $(OBJ_C) $(LD_OPTS) -o $(TARGET)

$(OBJ_C): $(OBJDIR)/%.o : %.c
	@$(CC) $(CFLAGS) -o $@ -c $<

clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(TARGET)

$(DEPS_C): $(OBJDIR)/%.d : %.c
	@mkdir -p $(OBJDIR); \
	set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJDIR)\/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS_C)
