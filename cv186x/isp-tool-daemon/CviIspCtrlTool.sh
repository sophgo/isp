#!/bin/sh

while getopts "hd" OPTION; do
    case $OPTION in
        d)
            if [ -z "$CVI_ENABLE_RAW_DUMP" ]; then
                export CVI_ENABLE_RAW_DUMP=1
            fi
            ;;
        h)
            echo "Usage:"
            echo "   -d     enable raw dump"
            echo "   -h     help (this output)"
            exit 0
            ;;
    esac
done

# for eaier debugging, add $PWD to LD_LIBRARY_PATH and PATH
SCRIPT_SELF=$(cd "$(dirname "$0")"; pwd)
export LD_LIBRARY_PATH=${SCRIPT_SELF}/lib:${SCRIPT_SELF}/lib/ai:${LD_LIBRARY_PATH}

PATH=${SCRIPT_SELF}:/mnt/system/usr/bin:$PATH
cd ${SCRIPT_SELF}
isp_tool_daemon_ctrl
