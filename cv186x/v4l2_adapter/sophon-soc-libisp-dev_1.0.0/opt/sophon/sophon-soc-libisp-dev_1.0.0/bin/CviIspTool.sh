#!/bin/sh

echo 16777216 > /proc/sys/net/core/wmem_max
echo "4096 873800 16777216" > /proc/sys/net/ipv4/tcp_wmem
echo "3073344 4097792 16777216" > /proc/sys/net/ipv4/tcp_mem

SCRIPT_SELF=$(cd "$(dirname "$0")"; pwd)
PATH=${SCRIPT_SELF}:$PATH

cd $SCRIPT_SELF
isp_tool_daemon
