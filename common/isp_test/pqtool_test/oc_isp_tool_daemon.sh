
#!/bin/bash

LOG_FILE="./tooldaemon.log"

./CviIspTool.sh </dev/null &>"$LOG_FILE" &

sleep 600

PID=$(ps -ef | grep '[i]sp_tool_daemon' |  grep -v grep | awk '{print $1}')


echo "isp_tool_daemon is $PID"

kill -9 "$PID"

wait "$PID"
status=$?
echo "Process exited with status $status"
