SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

#ISP_3A_CUSTOMIZE_AE = 1
#ISP_3A_CUSTOMIZE_AWB = 1
#ISP_3A_CUSTOMIZE_AF = 1

ifeq ($(ISP_3A_CUSTOMIZE_AE),1)
AE_LIB = custom_ae
else
AE_LIB = ae
endif

ifeq ($(ISP_3A_CUSTOMIZE_AF),1)
AF_LIB = custom_af
else
AF_LIB = af
endif

ifeq ($(ISP_3A_CUSTOMIZE_AWB),1)
AWB_LIB = custom_awb
else
AWB_LIB = awb
endif

.PHONY : clean all
.SILENT: clean all algo_ae algo_awb algo_af

all : algo_ae algo_awb algo_af

algo_ae:
	@if [ "$(ISP_3A_CUSTOMIZE_AE)" = "1" ]; then \
		cd $(AE_LIB) && make; \
	else \
		cp $(AE_LIB)/libae* $(MW_PATH)/lib/; \
	fi

algo_awb:
	@if [ "$(ISP_3A_CUSTOMIZE_AWB)" = "1" ]; then \
		cd $(AWB_LIB) && make; \
	else \
		cp $(AWB_LIB)/libawb* $(MW_PATH)/lib/; \
	fi

algo_af:
	@if [ "$(ISP_3A_CUSTOMIZE_AF)" = "1" ]; then \
		cd $(AF_LIB) && make; \
	else \
		cp $(AF_LIB)/libaf* $(MW_PATH)/lib; \
	fi


clean:
	@if [ "$(ISP_3A_CUSTOMIZE_AE)" = "1" ]; then \
		cd $(AE_LIB) && make clean; \
	else \
		rm $(MW_PATH)/lib/libae*; \
	fi
	@if [ "$(ISP_3A_CUSTOMIZE_AWB)" = "1" ]; then \
		cd $(AWB_LIB) && make; \
	else \
		rm $(MW_PATH)/lib/libawb*; \
	fi
	@if [ "$(ISP_3A_CUSTOMIZE_AF)" = "1" ]; then \
		cd $(AF_LIB) && make; \
	else \
		rm $(MW_PATH)/lib/libaf*; \
	fi
