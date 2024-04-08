SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=$(TOP_DIR)/middleware/$(MW_VER)/Makefile.param
	include $(PARAM_FILE)
endif

# add sensor -D definition
include $(BUILD_PATH)/.config
include Kbuild
ISP_HELPER_SENSOR_DEF += $(KBUILD_DEFINES)

MW_TARGET_SO = $(MW_LIB)/libispv4l2_adapter.so
MW_TARGET_A =  $(MW_LIB)/libispv4l2_adapter.a

ISP_TARGET_SO = $(MW_LIB)/libispv4l2_helper.so
ISP_TARGET_A =  $(MW_LIB)/libispv4l2_helper.a

SYS_SRCS = $(wildcard mw/sys/*.c)
VI_SRCS = $(wildcard mw/vi/*.c)
VO_SRCS = $(wildcard mw/vo/*.c)
VPSS_SRCS = $(wildcard mw/vpss/*.c)

MW_SRCS = $(SYS_SRCS) $(VI_SRCS) $(VO_SRCS) $(VPSS_SRCS)
MW_OBJS = $(MW_SRCS:.c=.o)

ISP_SRCS = $(wildcard isp/*.c)
ISP_OBJS = $(ISP_SRCS:.c=.o)


DEPS = $(MW_SRCS:.c=.d)
DEPS += $(ISP_SRCS:.c=.d)

VPSS_INC = $(MW_PATH)/modules/vpss/include

INCS += -Iinc
INCS += -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I$(VI_INC) -I$(VPSS_INC)
# drm for vo
DRM_INC = -I$(MW_INC)/3rd/drm -I$(MW_INC)/3rd/drm/libdrm
DRM_OBJ = $(TOP_DIR)/middleware/$(MW_VER)/lib/3rd/drm/libdrm.a

LOCAL_CFLAGS = $(INCS) -std=gnu11 -MMD -MP

.PHONY: all clean

all: $(MW_TARGET_A) $(MW_TARGET_SO) $(ISP_TARGET_SO) $(ISP_TARGET_A)

mw/sys/%.o : mw/sys/%.c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

mw/vi/%.o : mw/vi/%.c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

mw/vo/%.o : mw/vo/%.c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) $(DRM_INC) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

mw/vpss/%.o : mw/vpss/%.c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

isp/%.o : isp/%.c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) $(ISP_HELPER_SENSOR_DEF) -c $< -o $@
	@echo "------ isp helper build flag ------"
	@echo ">> CFLAGS: " $(CFLAGS)
	@echo ">> LOCAL_CFLAGS: " $(LOCAL_CFLAGS)
	@echo ">> sensor definition: " $(ISP_HELPER_SENSOR_DEF)

	@echo [$(notdir $(CC))] $(notdir $@)

$(MW_TARGET_A): $(MW_OBJS)
	$(AR) $(ARFLAGS) $@ $(MW_OBJS) $(DRM_OBJ)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(MW_TARGET_SO): $(MW_OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(MW_OBJS) $(DRM_OBJ) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

$(ISP_TARGET_A): $(ISP_OBJS)
	$(AR) $(ARFLAGS) $@ $(ISP_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(ISP_TARGET_SO): $(ISP_OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(ISP_OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

clean:
	-@rm $(MW_OBJS) 2>/dev/null || true
	-@rm $(ISP_OBJS) 2>/dev/null || true
	-@rm $(DEPS) 2>/dev/null || true
	-@rm $(MW_TARGET_A) $(MW_TARGET_SO) 2>/dev/null || true
	-@rm $(ISP_TARGET_A) $(ISP_TARGET_SO) 2>/dev/null || true

-include $(DEPS)
