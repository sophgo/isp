#!/bin/bash

CHIP_ID=$1

DATE=$(date +%Y%m%d)
#toolJsonGenerator repo
GENERATOR_CHANGE_ID="NULL"
GENERATOR_COMMIT_ID="NULL"
GENERATOR_V="(${GENERATOR_CHANGE_ID},${GENERATOR_COMMIT_ID})"
ISP_BRANCH="master"
#isp repo
cd $PWD/../../
ISP_CHANGE_ID="NULL"
ISP_COMMIT_ID="NULL"
ISP_V="(${ISP_CHANGE_ID},${ISP_COMMIT_ID})"
cd -
CUR_PATH=$(cd "$(dirname "$0")"; pwd)
DEVICE_FILE="${CUR_PATH}/device.json"
OUTPUT="pqtool_definition.json"
OUTPUT_TEMP="pqtool_definition_temp.json"
if [ $CHIP_ID == "cv181x" ] || [ $CHIP_ID == "cv180x" ] || [ $CHIP_ID == "cv186x" ]
then
    OSDRV_INCLUDE_PATH=${CUR_PATH}/../../../../../../osdrv/interdrv/v2/include/chip/${CHIP_ID}/uapi/linux
    MW_INCLUDE_PATH=${CUR_PATH}/../../../../../$MW_VER/include
    ISP_INCLUDE_PATH=${CUR_PATH}/../../include/$CHIP_ID
    RPCJSON_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2
    OUTPUT_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2/src/
else
    MW_INCLUDE_PATH=${CUR_PATH}/../../../../../$MW_VER/include
    ISP_INCLUDE_PATH=${CUR_PATH}/../../include/$CHIP_ID
    RPCJSON_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2
    OUTPUT_PATH=${CUR_PATH}/../../$CHIP_ID/isp-tool-daemon/
fi

OSDRV_COMMON_INCLUDE_PATH=${TOP_DIR}/osdrv/interdrv/${MW_VER}/include/common/uapi/linux
HEADERLIST="$ISP_INCLUDE_PATH/cvi_comm_isp.h"
HEADERLIST+=" $ISP_INCLUDE_PATH/cvi_comm_3a.h"
HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_sns.h"

if [ $CHIP_ID == "cv181x" ] || [ $CHIP_ID == "cv180x" ] || [ $CHIP_ID == "cv186x" ]
then
    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_vi.h"
    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_vpss.h"

    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_defines.h"
    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_video.h"
    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_vo.h"
else
    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_video.h"
    HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_vo.h"
fi
ADDHEADERLIST+=" $MW_INCLUDE_PATH/cvi_common.h"
ADDHEADERLIST+=" $MW_INCLUDE_PATH/cvi_type.h"

LEVELJSON=$CHIP_ID/level.json
LAYOUTJSON=$CHIP_ID/layout.json
RPCJSON=$RPCJSON_PATH/rpc.json

#reset&update device.json
sed -i 's/"FULL_NAME": ""/"FULL_NAME": "'"${CHIP_ID}"'"/g' ${DEVICE_FILE}
sed -i 's/"CODE_NAME": ""/"CODE_NAME": "'"${CHIP_ID}"'"/g' ${DEVICE_FILE}
sed -i 's/"SDK_VERSION": ""/"SDK_VERSION": "'"${DATE}"'"/g' ${DEVICE_FILE}
sed -i 's/"GENERATOR_VERSION": ""/"GENERATOR_VERSION": "'"${GENERATOR_V}"'"/g' ${DEVICE_FILE}
sed -i 's/"ISP_VERSION": ""/"ISP_VERSION": "'"${ISP_V}"'"/g' ${DEVICE_FILE}
sed -i 's/"ISP_BRANCH": ""/"ISP_BRANCH": "'"${ISP_BRANCH}"'"/g' ${DEVICE_FILE}



#generate pqtool_definition.json
cd ${CUR_PATH}
OUTPUTFILE=./output.txt
start=$(date +%s)
python preProcess.py $HEADERLIST $ADDHEADERLIST
end=$(date +%s)
runtime=$((end-start))
echo "preProcess.py take $runtime seconds"
start=$(date +%s)
g++ -ffunction-sections -fdata-sections -w values.cpp -o values&&./values>>${OUTPUTFILE}&&rm values.cpp&&rm values
end=$(date +%s)
runtime=$((end-start))
echo "cpp file build take $runtime seconds"
if [ $? -eq 0 ]; then
  echo "Executed cpp file successfully"
else
  echo "Executed cpp file erro!"
  exit -1
fi
start=$(date +%s)
python hFile2json.py  $LEVELJSON $LAYOUTJSON $RPCJSON $HEADERLIST
rm -rf ${OUTPUTFILE}
end=$(date +%s)
runtime=$((end-start))
echo "run h2Filejson.py take $runtime seconds"

#check json file validity
cat ${OUTPUT} | python -m json.tool 1>/dev/null 2>json_error
error=$(cat json_error)
if [ ${#error} -eq 0 ]
then
    rm -r json_error
else
    echo  "the json file is a invalid, error is : ${error}!!!"
    exit -1
fi

#cp pqtool_definition.json
if [ -f ${OUTPUT} ]
then
    if [ -x "$(command -v zip)" ]
    then
        zip ${OUTPUT_TEMP} ${OUTPUT}
        mv ${OUTPUT_TEMP} ${OUTPUT}
    fi

    if [ $CHIP_ID == "cv181x" ] || [ $CHIP_ID == "cv180x" ] || [ $CHIP_ID == "cv186x" ]
    then
        xxd -i ${OUTPUT} > ${OUTPUT_PATH}/cvi_pqtool_json.h
        echo "Success!! generate cvi_pqtool_json.h to src dir of isp-daemon2 done"
    else
        cp ${OUTPUT} ${OUTPUT_PATH}
        echo "Success!! mv pqtool_definition.json to isp-tool-daemon done"
    fi

    cp ${OUTPUT} ${OUTPUT_PATH}
    echo "Success!! mv pqtool_definition.json to isp-tool-daemon done"
else
    echo "FAIL!! please check gen pqtool_definition.json flow"
fi
