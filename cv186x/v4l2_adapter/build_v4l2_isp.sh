#! /bin/bash

ISP_V4L2_SRC_PATH=${TOP_DIR}/middleware/${MW_VER}/modules/isp/cv186x
ISP_V4L2_LIB_PATH=${TOP_DIR}/middleware/${MW_VER}/lib
ISP_V4L2_SH_PATH=${TOP_DIR}/middleware/${MW_VER}/modules/isp/cv186x/isp-tool-daemon-v4l2/CviIspTool.sh
ISP_V4L2_2D_PATH=${TOP_DIR}/middleware/${MW_VER}/modules/isp/cv186x/isp-tool-daemon-v4l2/isp_tool_daemon
ISP_V4L2_RT_PATH=${TOP_DIR}/middleware/${MW_VER}/modules/isp/cv186x/isp-tool-daemon-v4l2/prebuilt/libarm64/libcvi_rtsp.so
ISP_V4L2_UT_PATH=${TOP_DIR}/middleware/${MW_VER}/self_test/ispv4l2_ut/ispv4l2_ut

ISP_MW_VER="1.0.0"
DEB_PREFIX_DIR=${ISP_V4L2_SRC_PATH}/v4l2_adapter
DEB_SCRIPT_PATH=${ISP_V4L2_SRC_PATH}/v4l2_adapter/debian_script
DEB_DIR="${DEB_PREFIX_DIR}/sophon-soc-libisp_${ISP_MW_VER}"
DEB_PATH="${DEB_PREFIX_DIR}/sophon-soc-libisp_${ISP_MW_VER}_arm64.deb"
DEB_EDGE_LIB_PATH=${DEB_PREFIX_DIR}/edge_lib

# dev
DEB_DEV_DIR="${DEB_PREFIX_DIR}/sophon-soc-libisp-dev_${ISP_MW_VER}"
DEB_DEV_PATH="${DEB_PREFIX_DIR}/sophon-soc-libisp-dev_${ISP_MW_VER}_arm64.deb"
PY_SCRIPT_PATH=${DEB_SCRIPT_PATH}/parse_header.py

DEB_INSTALL_DIR=${DEB_DIR}/opt/sophon/sophon-soc-libisp_${ISP_MW_VER}
DEB_INSTALL_DEV_DIR=${DEB_DEV_DIR}/opt/sophon/sophon-soc-libisp-dev_${ISP_MW_VER}

test -d ${DEB_DIR} && rm -rf ${DEB_DIR}
test -d ${DEB_DEV_DIR} && rm -rf ${DEB_DEV_DIR}
test -f ${DEB_PATH} && rm -f ${DEB_PATH}
test -f ${DEB_DEV_PATH} && rm -f ${DEB_DEV_PATH}

DEB_DEBIAN_DIR="${DEB_DIR}/DEBIAN"; mkdir -p $DEB_DEBIAN_DIR

DEB_DIR_LIB=${DEB_INSTALL_DIR}/lib; mkdir -p $DEB_DIR_LIB
DEB_DIR_BIN=${DEB_INSTALL_DIR}/bin; mkdir -p $DEB_DIR_BIN
DEB_DIR_DATA=${DEB_INSTALL_DIR}/data; mkdir -p $DEB_DIR_DATA
DEB_DIR_DOC=${DEB_INSTALL_DIR}/doc; mkdir -p $DEB_DIR_DOC
DEB_DIR_SRC=${DEB_INSTALL_DIR}/src; mkdir -p $DEB_DIR_SRC
DEB_DIR_INC=${DEB_INSTALL_DIR}/include; mkdir -p $DEB_DIR_INC
DEB_DIR_SAMPLE=${DEB_INSTALL_DIR}/sample; mkdir -p $DEB_DIR_SAMPLE

export V4L2_ISP_ENABLE=1

ORIGINAL_LOCATION=$PWD

if [ -d $DEB_EDGE_LIB_PATH ]; then
	rm -rf $DEB_EDGE_LIB_PATH
fi

mkdir -p $DEB_EDGE_LIB_PATH

echo "build 3rd party ..."
build_3rd_party || return $?
cd ${TOP_DIR}/middleware/${MW_VER}/3rdparty || return $?
make clean &> /dev/null
make all || return $?

echo "build sensor ..."
cd ${TOP_DIR}/middleware/${MW_VER}/component/isp
make clean &> /dev/null
make all || return $?
cp ${ISP_V4L2_LIB_PATH}/libsns_full.so ${DEB_EDGE_LIB_PATH}/


echo "build cvi bin ..."
cd ${TOP_DIR}/middleware/${MW_VER}/modules/bin || return $?
make clean &> /dev/null
make all || return $?
cp ${ISP_V4L2_LIB_PATH}/libcvi_bin.so ${DEB_EDGE_LIB_PATH}/

echo "build cvi venc ..."
cd ${TOP_DIR}/middleware/${MW_VER}/modules/vcodec || return $?
make clean &> /dev/null
make all || return $?
cp ${ISP_V4L2_LIB_PATH}/libvenc.so ${DEB_EDGE_LIB_PATH}/

echo "build isp ..."
cd $ISP_V4L2_SRC_PATH || return $?
make clean &> /dev/null
make all || return $?

# build isp-tool-daemon-v4l2
cd $ISP_V4L2_SRC_PATH/isp-tool-daemon-v4l2 || return $?
make clean &> /dev/null
make all || return $?

echo "build ispv4l2_ut ..."
cd ${TOP_DIR}/middleware/${MW_VER}/self_test/ispv4l2_ut || return $?
make clean &> /dev/null
make all || return $?

# clean the shared so
cd ${TOP_DIR}/middleware/${MW_VER}/component/isp
make clean &> /dev/null

cd ${TOP_DIR}/middleware/${MW_VER}/modules/bin || return $?
make clean &> /dev/null

cd ${TOP_DIR}/middleware/${MW_VER}/modules/vcodec || return $?
make clean &> /dev/null

cd $ORIGINAL_LOCATION

export V4L2_ISP_ENABLE=0

# pack lib
ISP_LIBS=(
    libisp.so
    libae.so
    libawb.so
    libaf.so
    libisp_algo.so
    libcvi_ispd2.so
    libispv4l2_adapter.so
    libispv4l2_helper.so
    libteaisp.so
)

ISP_EDGE_LIBS=(
	libsns_full.so
	libvenc.so
	libcvi_bin.so
)

echo "pack isp lib ..."
for lib in ${ISP_LIBS[@]}
do
    lib_path=${ISP_V4L2_LIB_PATH}/${lib}
    if [ ! -f $lib_path ]; then
        echo "$lib_path not existed! Exit ..."
        return 1
    else
        cp $lib_path $DEB_DIR_LIB
    fi
done

for lib in ${ISP_EDGE_LIBS[@]}
do
    lib_path=${DEB_EDGE_LIB_PATH}/${lib}
    if [ ! -f $lib_path ]; then
        echo "$lib_path not existed! Exit ..."
        return 1
    else
        cp $lib_path $DEB_DIR_LIB
    fi
done

rm -rf ${DEB_EDGE_LIB_PATH} &> /dev/null

# empty so for linking
CVI_BIN_ISP_LIB=libcvi_bin_isp.so
if [ -f ${ISP_V4L2_LIB_PAHT}/${CVI_BIN_ISP_LIB} ]; then
	cp ${ISP_V4L2_LIB_PAHT}/${CVI_BIN_ISP_LIB} $DEB_DIR_LIB
else
	echo -e "void __dummy_cvi_bin_isp__() {}" | ${CROSS_COMPILE}gcc -x c -shared -o ${CVI_BIN_ISP_LIB} -fPIC -
	if [ $? -eq 0 ]; then
		cp ${CVI_BIN_ISP_LIB} $DEB_DIR_LIB
		rm ${CVI_BIN_ISP_LIB}
	else
		echo "fail to generate the ${CVI_BIN_ISP_LIB}"
		return 1
	fi
fi

echo "pack 3rd party lib ..."
if [ ! -d $ISP_V4L2_LIB_PATH/3rd ]; then
    echo "${ISP_V4L2_LIB_PATH}/3rd not existed! Exit ..."
    return 1
else
    # cp -L ${ISP_V4L2_LIB_PATH}/3rd/libjson-c* ${ISP_V4L2_LIB_PATH}/3rd/libini.so $DEB_DIR_LIB
    cp -L ${ISP_V4L2_LIB_PATH}/3rd/libini.so $DEB_DIR_LIB
fi

cp $ISP_V4L2_RT_PATH $DEB_DIR_LIB

# pack bin
echo "pack bin ..."
if [ ! -f $ISP_V4L2_UT_PATH ]; then
    echo "$ISP_V4L2_UT_PATH not existed! Exit ..."
    return 1
else
    cp $ISP_V4L2_UT_PATH $DEB_DIR_BIN
fi

if [ ! -f $ISP_V4L2_2D_PATH ]; then
    echo "$ISP_V4L2_2D_PATH not existed! Exit ..."
    return 1
else
    cp $ISP_V4L2_2D_PATH $DEB_DIR_BIN
fi

cp $ISP_V4L2_SH_PATH $DEB_DIR_BIN
cp $ISP_V4L2_SRC_PATH/isp-tool-daemon-v4l2/cfg.json $DEB_DIR_BIN
cp $ISP_V4L2_SRC_PATH/isp-tool-daemon-v4l2/vc_param.json $DEB_DIR_BIN
cp -r ${TOP_DIR}/cvi_rtsp/cvi_models $DEB_DIR_BIN

# pack doc
cp ${ISP_V4L2_SRC_PATH}/isp-tool-daemon-v4l2/README.md $DEB_DIR_DOC

# pack src
cp -r ${DEB_PREFIX_DIR}/isp $DEB_DIR_SRC
cp -rL ${TOP_DIR}/middleware/v2/component/isp/sensor/cv186x $DEB_DIR_SRC/sensor
cp -rL ${TOP_DIR}/middleware/v2/component/isp/sensor.mk $DEB_DIR_SRC
cp -r ${BUILD_PATH}/.config $DEB_DIR_SRC
cp -r ${DEB_PREFIX_DIR}/Kbuild $DEB_DIR_SRC
cp -r ${DEB_PREFIX_DIR}/Makefile.release ${DEB_DIR_SRC}/Makefile

# pack sample
ISP_V4L2_TOOL_DAEMON_DIR=${ISP_V4L2_SRC_PATH}/isp-tool-daemon-v4l2
cp -r ${ISP_V4L2_TOOL_DAEMON_DIR}/{inc,prebuilt,src} $DEB_DIR_SAMPLE
cp -rp ${ISP_V4L2_TOOL_DAEMON_DIR}/{cfg.json,CviIspTool.sh,vc_param.json} $DEB_DIR_SAMPLE
cp -r ${TOP_DIR}/middleware/${MW_VER}/lib/3rd/drm ${DEB_DIR_SAMPLE}/prebuilt/
cp -r ${ISP_V4L2_TOOL_DAEMON_DIR}/Makefile.release ${DEB_DIR_SAMPLE}/Makefile

# deal with sample header
for c_src_file_d in ${ISP_V4L2_SRC_PATH}/isp-tool-daemon-v4l2/tmp/*.d
do
    python3 $PY_SCRIPT_PATH ${c_src_file_d} ${ISP_V4L2_SRC_PATH}/isp-tool-daemon-v4l2 $DEB_DIR_SAMPLE/inc
done

# pack header files
cp -r ${DEB_PREFIX_DIR}/inc/cvi_isp_v4l2.h $DEB_DIR_INC
python3 $PY_SCRIPT_PATH ${DEB_PREFIX_DIR}/isp/isp.d ${DEB_PREFIX_DIR} $DEB_DIR_INC/inc
for dir_file in $(ls ${DEB_DIR_SRC}/sensor)
do
    dir_file_path=${DEB_DIR_SRC}/sensor/${dir_file}
    echo "deal: ${dir_file_path}"
    if [ -d "$dir_file_path" ]; then
        for d_file in ${dir_file_path}/*.d
        do
            if [ -e "$d_file" ]; then
                python3 $PY_SCRIPT_PATH $d_file ${dir_file_path} $DEB_DIR_INC/inc
                rm -f $d_file
            fi
        done
        rm -f ${dir_file_path}/Makefile
        rm -f ${dir_file_path}/*.o
        # copy the Makefile.sensor
        cp -r ${DEB_PREFIX_DIR}/Makefile.sensor ${dir_file_path}/Makefile
    else
        rm -f $dir_file_path
    fi
done

# pack bin
ISP_TUNING_PATH=${TOP_DIR}/isp_tuning/sophon/src
cp -rf ${ISP_TUNING_PATH}/* $DEB_DIR_DATA

# generate the dev deb package
cp -rf $DEB_DIR $DEB_DEV_DIR

# generate the deb package
cp -f ${DEB_SCRIPT_PATH}/sophon-isp/control ${DEB_DEBIAN_DIR}
cp -f ${DEB_SCRIPT_PATH}/sophon-isp/prerm ${DEB_DEBIAN_DIR}
cp -f ${DEB_SCRIPT_PATH}/sophon-isp/postinst ${DEB_DEBIAN_DIR}
cp -f ${DEB_SCRIPT_PATH}/sophon-isp/postrm ${DEB_DEBIAN_DIR}
cp -f ${DEB_SCRIPT_PATH}/sophon-isp/sophon-isp-autoconf.sh ${DEB_DIR_DATA}
cp -f ${DEB_SCRIPT_PATH}/sophon-isp/sophon-isp.conf ${DEB_DIR_DATA}

rm -rf $DEB_DIR_SRC $DEB_DIR_INC $DEB_DIR_SAMPLE
dpkg -b $DEB_DIR $DEB_PATH
dpkg -c $DEB_PATH

# generate the dev deb package
mv ${DEB_DEV_DIR}/opt/sophon/sophon-soc-libisp{,-dev}_${ISP_MW_VER}

cp -f ${DEB_SCRIPT_PATH}/sophon-isp-dev/control ${DEB_DEV_DIR}/DEBIAN/
cp -f ${DEB_SCRIPT_PATH}/sophon-isp-dev/prerm ${DEB_DEV_DIR}/DEBIAN/
cp -f ${DEB_SCRIPT_PATH}/sophon-isp-dev/postinst ${DEB_DEV_DIR}/DEBIAN/
cp -f ${DEB_SCRIPT_PATH}/sophon-isp-dev/postrm ${DEB_DEV_DIR}/DEBIAN/
cp -f ${DEB_SCRIPT_PATH}/sophon-isp-dev/sophon-isp-dev-autoconf.sh ${DEB_INSTALL_DEV_DIR}/data
cp -f ${DEB_SCRIPT_PATH}/sophon-isp-dev/sophon-isp-dev.conf ${DEB_INSTALL_DEV_DIR}/data

rm -rf ${DEB_DEV_DIR}/opt/sophon/sophon-soc-libisp-dev_${ISP_MW_VER}/lib/*
cp ${ISP_V4L2_SRC_PATH}/isp-tool-daemon-v4l2/README.md.release ${DEB_INSTALL_DEV_DIR}/doc/README.md
dpkg -b $DEB_DEV_DIR $DEB_DEV_PATH
dpkg -c $DEB_DEV_PATH

