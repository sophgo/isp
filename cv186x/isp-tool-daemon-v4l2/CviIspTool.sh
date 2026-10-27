#!/bin/sh

echo 16777216 > /proc/sys/net/core/wmem_max
echo "4096 873800 16777216" > /proc/sys/net/ipv4/tcp_wmem
echo "3073344 4097792 16777216" > /proc/sys/net/ipv4/tcp_mem

SCRIPT_SELF=$(cd "$(dirname "$0")"; pwd)
PATH=${SCRIPT_SELF}:$PATH

getopts_get_optional_argument() {
    eval next_token=\${$OPTIND}
    if [ -n $next_token ] && [ $next_token != -* ]; then
        OPTIND=$((OPTIND + 1))
        OPTARG=$next_token
    else
        OPTARG=""
    fi
}

while getopts "ht" OPTION; do
    case $OPTION in
        t)
            getopts_get_optional_argument $@
            echo "start isp auto test case: $OPTARG"
            export CVI_ISP_AUTO_TEST_CASE="$OPTARG"
            export PQBIN_DIR="/mnt/sd/res_a2/pqbin"
            ;;
        h)
            echo "Usage:"
            echo "   -t N     start isp auto test case number N"
            exit 0
            ;;
    esac
done

export LD_LIBRARY_PATH=${SCRIPT_SELF}../lib:${LD_LIBRARY_PATH}

cd $SCRIPT_SELF
isp_tool_daemon
