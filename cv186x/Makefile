.PHONY: clean all
.SILENT: clean all

ISP_BASE_PATH = $(PWD)/..
ISP_COMMON_DIR = $(ISP_BASE_PATH)/common
ISP_3A_DIR = $(ISP_BASE_PATH)/algo
CV182X_BASE_PATH = $(ISP_BASE_PATH)/cv182x
#CV182X_COMMON_BASE_PATH = $(CV182X_BASE_PATH)/common
CV183X_BASE_PATH = $(ISP_BASE_PATH)/cv183x
#CV183X_COMMON_BASE_PATH = $(CV183X_BASE_PATH)/common

# v4l2 use files: vi_v4l2_uapi.h, sns_v4l2_uapi.h
ifeq ($(PARAM_FILE),)
	PARAM_FILE := $(TOP_DIR)/middleware/$(MW_VER)/Makefile.param
	include $(PARAM_FILE)
endif

all:
	@cd isp_algo; $(MAKE) all;
	@cd $(ISP_3A_DIR); $(MAKE) all;
	@cd isp; $(MAKE) all;
	@cd teaisp; $(MAKE) all;
	@cd isp_bin; $(MAKE) all;
	@cd $(ISP_COMMON_DIR)/toolJsonGenerator; ./generate_toolJson.sh $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
	@cd isp-daemon2; $(MAKE) -j4; $(MAKE) install;
	@cd $(ISP_COMMON_DIR)/raw_replay; $(MAKE) all;
	@cd $(ISP_COMMON_DIR)/raw_dump; $(MAKE) all;
ifeq ($(V4L2_ISP_ENABLE),1)
	@cd v4l2_adapter; $(MAKE) all;
endif

ifeq ($(ISP_SRC_RELEASE),1)
	# common base
	# Remove not released folder (in cv182x)
	-@rm -rf $(CV182X_COMMON_BASE_PATH)/inc $(CV182X_COMMON_BASE_PATH)/isp-rpc-tool-daemon $(CV182X_COMMON_BASE_PATH)/rpc
	-@rm -rf $(CV182X_COMMON_BASE_PATH)/raw_dump_test $(CV182X_COMMON_BASE_PATH)/raw_replay_test

	# Remove not released folder (in cv183x)
	-@rm -rf $(CV183X_COMMON_BASE_PATH)/inc $(CV183X_COMMON_BASE_PATH)/isp-rpc-tool-daemon $(CV183X_COMMON_BASE_PATH)/rpc
	-@rm -rf $(CV183X_COMMON_BASE_PATH)/raw_dump_test $(CV183X_COMMON_BASE_PATH)/raw_replay_test

	# Remove other SOCs (cv183x)
	-@rm -f $(CV183X_BASE_PATH)/Makefile*
	-@rm -rf $(CV183X_BASE_PATH)/include $(CV183X_BASE_PATH)/src
	-@rm -rf $(CV183X_BASE_PATH)/isp-daemon $(CV183X_BASE_PATH)/isp-tool-daemon
	-@rm -rf $(CV183X_BASE_PATH)/isp_bin $(CV183X_BASE_PATH)/isp_test
	-@rm -rf $(ISP_BASE_PATH)/include/cv183x

	# Replace makefile
	-@mv Makefile.sdk_release Makefile
endif

clean:
	@cd isp-daemon2; $(MAKE) clean;
	@cd isp; $(MAKE) clean;
	@cd teaisp; $(MAKE) clean;
	@cd isp_bin; $(MAKE) clean;
	@cd $(ISP_3A_DIR); $(MAKE) clean;
	@cd isp_algo; $(MAKE) clean;
	@cd $(ISP_COMMON_DIR)/raw_replay; $(MAKE) clean;
	@cd $(ISP_COMMON_DIR)/raw_dump; $(MAKE) clean;
ifeq ($(V4L2_ISP_ENABLE),1)
	@cd v4l2_adapter; $(MAKE) clean;
endif
