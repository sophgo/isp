SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

include ../../isp.mk

BASE_DIR = $(PWD)
ISP_DIR = $(BASE_DIR)/..
ISP_COMMON_DIR = $(BASE_DIR)/../../common
ISP_3A_DIR = $(BASE_DIR)/../../algo

# 3rdparty lib
TRD_LIB_INCLUDE_PATH = $(PWD)/prebuilt/include
TRD_LIB_PATH = $(PWD)/prebuilt/$(SDK_VER)

INC_DIRS = $(KERNEL_INC) $(MW_INC) $(ISP_INC) $(VI_INC) $(ISP_DIR)/isp/inc
INC_DIRS += $(BASE_DIR)/inc $(BASE_DIR)/src $(BASE_DIR)/json_struct
INC_DIRS += $(ISP_COMMON_DIR)/raw_replay
INC_DIRS += $(ISP_3A_DIR)
INC_DIRS += $(ISP_COMMON_DIR)/isp_ipc
INC_DIRS += $(BIN_INC) $(SYS_INC)
INC_DIRS += $(TRD_LIB_INCLUDE_PATH)/cvi_json-c

LIB_DIRS = $(MW_LIB) $(TRD_LIB_PATH)

CINCS = $(foreach incdir, $(INC_DIRS), -I$(incdir))
LINCS = $(foreach libdir, $(LIB_DIRS), -L$(libdir))

TMP_FOLDER = tmp
OUT_FOLDER = build
RELEASE_FOLDER_BIN = $(PWD)/bin

#USRDEFS = -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET_A = libcvi_ispd2.a
TARGET_SO = libcvi_ispd2.so

LOCAL_CFLAGS = $(CINCS) $(USRDEFS) -rdynamic -fPIC
#LOCAL_CFLAGS += $(DEPFLAGS)
LOCAL_LDFLAGS = $(LINCS) $(LIBS) -shared

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

CFLAGS += -Wno-format-truncation

.PHONY: all clean cleanall install uninstall upload prepare_3rdparty_module
.SILENT: clean cleanall install uninstall upload prepare_3rdparty_module

OBJS =	$(patsubst src/%.c, $(TMP_FOLDER)/%.o, $(filter-out $(EXCLUDES), $(wildcard src/*.c))) \
		$(patsubst json_struct/%.c, $(TMP_FOLDER)/%.o, $(wildcard json_struct/*.c)) \
		$(patsubst %.c, $(TMP_FOLDER)/%.o, $(wildcard *.c))
DEPS =	$(patsubst src/%.c, $(TMP_FOLDER)/%.d, $(filter-out $(EXCLUDES), $(wildcard src/*.c))) \
		$(patsubst json_struct/%.c, $(TMP_FOLDER)/%.d, $(wildcard json_struct/*.c)) \
		$(patsubst %.c, $(TMP_FOLDER)/%.d, $(wildcard *.c))

#ifeq ($(ISP_SRC_RELEASE),1)
#all: $(TARGET_A) $(TARGET_SO) sdk_release
#else
all: $(TARGET_A) $(TARGET_SO)
#endif

$(TMP_FOLDER)/%.o: $(BASE_DIR)/src/%.c $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TMP_FOLDER)/%.o: $(BASE_DIR)/%.c $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TMP_FOLDER)/%.o: $(BASE_DIR)/json_struct/%.c $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) $(OUT_FOLDER)
	@$(AR) $(ARFLAGS) $(OUT_FOLDER)/$(TARGET_A) $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@cp $(BASE_DIR)/inc/cvi_ispd2.h $(OUT_FOLDER)
	@cp -a $(TRD_LIB_PATH)/libcvi_json-c.a $(OUT_FOLDER)

$(TARGET_SO): $(OBJS) | prepare_3rdparty_module $(OUT_FOLDER)
	@$(LD) $(LDFLAGS) $(LOCAL_LDFLAGS) -o $(OUT_FOLDER)/$(TARGET_SO) --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))
	@cp $(BASE_DIR)/inc/cvi_ispd2.h $(OUT_FOLDER)
	@cp -a $(TRD_LIB_PATH)/libcvi_json-c.a $(OUT_FOLDER)

$(TMP_FOLDER):
	@echo "Create $(TMP_FOLDER) for object file"
	-@mkdir -p $(TMP_FOLDER)

$(OUT_FOLDER):
	@echo "Create $(OUT_FOLDER) for output"
	-@mkdir -p $(OUT_FOLDER)

upload:
	@sshpass -p "cvitek" scp $(OUT_FOLDER)/$(TARGET_SO) root@192.168.1.3:/mnt/data/install/lib
	@echo "Done"

install:
	@cp $(OUT_FOLDER)/$(TARGET_A) $(MW_LIB)
	@cp $(OUT_FOLDER)/$(TARGET_SO) $(MW_LIB)
	@cp -a $(OUT_FOLDER)/libcvi_json-c.a $(MW_3RD_LIB)

uninstall:
	-@rm -f $(MW_LIB)/$(TARGET_A)
	-@rm -f $(MW_LIB)/$(TARGET_SO)

sdk_release: $(TARGET_A) $(TARGET_SO)
	@echo "Create $(RELEASE_FOLDER_BIN)"

	-@mkdir -p $(RELEASE_FOLDER_BIN)
	-@rm -rf $(RELEASE_FOLDER_BIN)/*
	-@cp $(OUT_FOLDER)/$(TARGET_A) $(RELEASE_FOLDER_BIN)
	-@cp $(OUT_FOLDER)/$(TARGET_SO) $(RELEASE_FOLDER_BIN)
	-@cp -a $(OUT_FOLDER)/libcvi_json-c.* $(RELEASE_FOLDER_BIN)

	-@rm -f cppcheck.sh .gitignore .vscode
	-@rm -rf $(TMP_FOLDER)
	-@rm -rf src $(OUT_FOLDER)

	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(OBJS) $(DEPS)
	@rm -f $(OUT_FOLDER)/$(TARGET_SO)
	@rm -f $(OUT_FOLDER)/$(TARGET_A)
	@rm -f $(OUT_FOLDER)/*.h
	@rm -f $(OUT_FOLDER)/libcvi_json-c.*
	-@rm -f $(MW_LIB)/$(TARGET_SO)
	-@rm -f $(MW_LIB)/$(TARGET_A)

cleanall: clean
	@rm -rf $(TMP_FOLDER) $(OUT_FOLDER)
	@rm -rf $(RELEASE_FOLDER_BIN)

-include $(DEPS)
