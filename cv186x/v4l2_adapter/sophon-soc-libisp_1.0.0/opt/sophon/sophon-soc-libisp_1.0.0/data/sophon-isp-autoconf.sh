export V4L2_ISP_INSTALL=/opt/sophon/sophon-soc-libisp_1.0.0
export V4L2_ISP_LIB=${V4L2_ISP_INSTALL}/lib
export V4L2_ISP_BIN=${V4L2_ISP_INSTALL}/bin
export V4L2_ISP_DOC=${V4L2_ISP_INSTALL}/doc
export V4L2_ISP_DATA=${V4L2_ISP_INSTALL}/data

export PATH=${V4L2_ISP_BIN}:$PATH
export PKG_CONFIG_PATH=${V4L2_ISP_LIB}/pkgconfig:$PKG_CONFIG_PATH

ffmpeg_set_isp_env()
{
    sudo chown root:kmem /dev/mem
    sudo chmod a+rw /dev/mem
    sudo chmod a+rw /dev/v4l*
    sudo setcap cap_sys_rawio+ep /opt/sophon/sophon-ffmpeg-latest/bin/ffmpeg
}

ispv4l2_ut_set_isp_env()
{
    sudo chown root:kmem /dev/mem
    sudo chmod a+rw /dev/mem
    sudo chmod a+rw /dev/v4l*
    sudo setcap cap_sys_rawio+ep ${V4L2_ISP_BIN}/ispv4l2_ut
}
