#!/bin/sh

DEFAULT_HOST="192.168.1.3"

echo 16777216 > /proc/sys/net/core/wmem_max
echo "4096 873800 16777216" > /proc/sys/net/ipv4/tcp_wmem
echo "3073344 4097792 16777216" > /proc/sys/net/ipv4/tcp_mem

CFG_JSON_FILE="./cfg.json"

if [ -z "$CVI_RTSP_JSON" ]; then
    export CVI_RTSP_JSON=$CFG_JSON_FILE
fi

getopts_get_optional_argument() {
    eval next_token=\${$OPTIND}
    if [[ -n $next_token && $next_token != -* ]]; then
        OPTIND=$((OPTIND + 1))
        OPTARG=$next_token
    else
        OPTARG=""
    fi
}

sed -i 's/"dev-num": 2/"dev-num": 1/g' $CFG_JSON_FILE
sed -i 's/"replay-mode": true/"replay-mode": false/g' $CFG_JSON_FILE
while getopts "hgmir" OPTION; do
    case $OPTION in
        i)
            getopts_get_optional_argument $@
            if [ -z "$OPTARG" ]; then
                HOST=${DEFAULT_HOST}
            else
                HOST=$OPTARG
            fi
            echo "set the IP address $HOST to network interface"
            ;;
        g)
            GIGABIT="true"
            echo "use gigabit ethernet"
            ;;
        m)
            if [ -z "$CVI_RTSP_MODE" ]; then
                export CVI_RTSP_MODE=1
            fi
            echo "use multi rtsp server"
            sed -i 's/"dev-num": 1/"dev-num": 2/g' $CFG_JSON_FILE
            ;;
        r)
            if [ -z "$CVI_REPLAY_MODE" ]; then
                export CVI_REPLAY_MODE=1
            fi
            echo "start replay mode"
            sed -i 's/"replay-mode": false/"replay-mode": true/g' $CFG_JSON_FILE
            ;;
        h)
            echo "Usage:"
            echo "   -i     set the IP address to network interface"
            echo "   -g     use gigabit ethernet"
            echo "   -m     use multi rtsp server"
            echo "   -h     help (this output)"
            exit 0
            ;;
    esac
done

# disable vcodec debug message to minize latency
echo 0x20001 > /sys/module/cv186x_vcodec/parameters/vcodec_mask
# enable remap
echo 1 > /sys/module/cvi_vc_driver/parameters/addrRemapEn
echo 256 > /sys/module/cvi_vc_driver/parameters/ARExtraLine

if [ "$HOST" ]; then
    if [ "$GIGABIT" == "true" ]; then
        # enable gigabit ethernet (=eth1)
        ifconfig eth0 down
        ifconfig eth1 up
        ifconfig eth1 $HOST netmask 255.255.255.0
        #udhcpc -b -i eth1 -R & # unmakr this line to use DHCP
    else
        ifconfig eth1 down
        ifconfig eth0 up
        ifconfig eth0 $HOST netmask 255.255.255.0
        #udhcpc -b -i eth1 -R & # unmark this line to use DHCP
    fi
fi

# for eaier debugging, add $PWD to LD_LIBRARY_PATH and PATH
SCRIPT_SELF=$(cd "$(dirname "$0")"; pwd)
export LD_LIBRARY_PATH=${SCRIPT_SELF}/lib:${SCRIPT_SELF}/lib/ai:${LD_LIBRARY_PATH}:/mnt/system/usr/lib:/mnt/system/usr/lib/3rd:/lib/3rd

PATH=${SCRIPT_SELF}:/mnt/system/usr/bin:$PATH
cd ${SCRIPT_SELF}
isp_tool_daemon
