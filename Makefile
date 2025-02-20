.PHONY: clean all
.SILENT: clean all

include isp.mk

all:
	if [ "$(wildcard $(isp_chip_dir))" != "" ]; then cd $(isp_chip_dir) ; $(MAKE) clean &>/dev/null; $(MAKE) all || exit 1; fi;

clean:
	if [ "$(wildcard $(isp_chip_dir))" != "" ]; then cd $(isp_chip_dir) ; $(MAKE) clean || exit 1; fi;
