SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

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
	cd $(AE_LIB); make;

algo_awb:
	cd $(AWB_LIB); make;

algo_af:
	cd $(AF_LIB); make;

clean:
	cd $(AE_LIB); make clean
	cd $(AWB_LIB); make clean
	cd $(AF_LIB); make clean
