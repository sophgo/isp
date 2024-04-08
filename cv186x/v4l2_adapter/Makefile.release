SHELL = /bin/bash

# add sensor -D definition
include .config
include Kbuild
include sensor.mk

ISP_TARGET_SO = ../lib/libispv4l2_helper_dev.so
SNS_TARGET_SO = ../lib/libsns_full_dev.so

ISP_SRCS = $(wildcard isp/*.c)
ISP_OBJS = $(ISP_SRCS:.c=.o)

SNS_SRCS = $(shell cd sensor && find $(sensor-y) -name '*.c')
SNS_SRCS := $(addprefix sensor/, $(SNS_SRCS))
SNS_OBJS = $(SNS_SRCS:.c=.o)

DEPS = $(ISP_SRCS:.c=.d)

INCS += -I../include -I../include/inc

LOCAL_CFLAGS = $(INCS) -std=gnu11 -MMD -MP

LDFLAGS = -shared -fPIC --gc-sections -export-dynamic

ISP_HELPER_SENSOR_DEF += $(KBUILD_DEFINES)

BUILD_SNS_TARGET := $(shell touch SNS_TARGET_TMP && echo SNS_TARGET_TMP)


.PHONY: all clean

all: $(ISP_TARGET_SO) $(SNS_TARGET_SO)

define build_sensor
	@echo "build sesnor object"
	@echo $(sensor-y)
	@for sensor_dir in $(sensor-y); \
	do \
		export PROJ_CFLAGS=-DF10; \
		echo "build " $$sensor_dir; \
		pushd $$PWD; \
		cd sensor/$$sensor_dir; \
		make; \
		popd; \
		unset PROJ_CFLAGS; \
	done
endef

isp/%.o : isp/%.c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) $(ISP_HELPER_SENSOR_DEF) -c $< -o $@
	@echo "------ isp helper build flag ------"
	@echo ">> CFLAGS: " $(CFLAGS)
	@echo ">> LOCAL_CFLAGS: " $(LOCAL_CFLAGS)
	@echo ">> sensor definition: " $(ISP_HELPER_SENSOR_DEF)
	@echo [$(notdir $(CC))] $(notdir $@)


$(ISP_TARGET_SO): $(ISP_OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(ISP_OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(ISP_TARGET_SO))

$(SNS_TARGET_SO): $(BUILD_SNS_TARGET)
	$(call build_sensor)
	$(LD) $(LDFLAGS) -o $@ --start-group $(SNS_OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(SNS_TARGET_SO))
	@rm $(BUILD_SNS_TARGET)


clean:
	-@rm $(ISP_OBJS)
	-@rm $(DEPS)
	-@rm $(ISP_TARGET_SO)
	-@rm sensor/*/*.o
	-@rm ../lib/*sns*

-include $(DEPS)
