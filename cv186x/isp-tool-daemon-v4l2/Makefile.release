SHELL = /bin/bash

TMP_FOLDER = $(PWD)/tmp
SDIR = $(PWD)/src

SRCS = $(wildcard $(SDIR)/*.c)
SRCS := $(filter-out $(SDIR)/self_test.c, $(SRCS))

INCS += -I./inc
INCS += -I../include

OBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(SRCS))
DEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(SRCS))

TARGET = isp_tool_daemon

EXTRA_CFLAGS = $(INCS)

ifeq ($(V4L2_ISP_LIB),)
	V4L2_ISP_LIB = $(wildcard /opt/sophon/*isp*/lib)
endif

V4L2_ISP_LIB := $(addprefix -L, $(V4L2_ISP_LIB))

ELFFLAGS += $(V4L2_ISP_LIB)
ELFFLAGS += -L../lib
ELFFLAGS += -L./prebuilt/libarm64
ELFFLAGS += -L./prebuilt/drm

LIBS += -lispv4l2_adapter -ldrm
LIBS += -lisp -lawb -lae -laf -lisp_algo -lcvi_ispd2 -lvenc -lcvi_rtsp
LIBS += -lispv4l2_helper_dev -lsns_full_dev -lcvi_bin_isp -lcvi_bin -ljson-c
ELFFLAGS += $(LIBS) -lpthread -lm -latomic -ldl

.PHONY : clean all self_test

all: $(TARGET)

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(OBJS)
	$(CXX) -o $@ -Wl,--start-group $(OBJS) -Wl,--end-group -Wl,--gc-sections -rdynamic $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER):
	@echo "Create $(TMP_FOLDER) for object file"
	-@mkdir -p $(TMP_FOLDER)

clean:
	-@rm -f $(OBJS) $(DEPS) $(TARGET)
	-@rm -rf $(TMP_FOLDER)

self_test:
	@$(CC) $(INCS) $(SDIR)/self_test.c $(SDIR)/utils.c -o self_test $(ELFFLAGS)

-include $(DEPS)
