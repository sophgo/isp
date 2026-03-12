SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

T_A_NAME = libisp_algo.a
T_SO_NAME = libisp_algo.so

TARGET_A = $(MW_LIB)/$(T_A_NAME)
TARGET_SO = $(MW_LIB)/$(T_SO_NAME)

.PHONY: all clean
.SILENT: all clean

all:
	@cp -f $(T_A_NAME) $(T_SO_NAME) $(MW_LIB)

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)
