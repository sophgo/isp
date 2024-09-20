# isp for edge device

## tutorial
1. 设置ip，ifconfig eth0  ip
    例如：ifconfig eth0 192.168.1.3

2. 执行 sudo -i 切到 root 用户

3. 安装sensor驱动
    例如：insmod /mnt/system/ko/v4l2_imx585.ko; insmod /mnt/system/ko/v4l2_os04a10_s1.ko

4. 配置codec参数
    (1) 进入到isp安装目录下的 bin 目录：
    ```shell
    cd /opt/sophon/*isp*/bin
    ```
    (2) 按需修改修改cfg.json和vc_param.json参数，特别注意cfg.json中的"dev-num"，单目设置为1，双目设置为2；
    (3) 如果需要hdmi外接显示器，cfg.json中的对应输出channel的"enable-hdmi"设置为true，否则为false。

5. pqbin加载设置
    首先需要创建目录/mnt/cfg/param，然后将pqbin拷贝到该目录，并按照是否为wdr设置为不同的文件名。
    (1) linear模式pqbin的路径为：/mnt/cfg/param/cvi_sdr_bin
    例如：对于一个命名为"os04a10_linear.bin"的pqbin，设置为指定路径：

    ```shell
    mkdir -p /mnt/cfg/param
    cp os04a10_linear.bin /mnt/cfg/param/cvi_sdr_bin
    ```
    (2) wdr模式下pqbin的路径为：/mnt/cfg/param/cvi_wdr_bin, 设置方法参考上面

6. 执行 CviIspTool.sh，任意位置皆可

## more tips
1. isp deb包的生成、卸载和重新安装
    若要修改isp相关的代码重新重新生成deb包和在板端使用，可按照下面的流程执行：

    ```shell
    # 编译isp deb包，生成的deb在 ${TOP_DIR}/middleware/v1/modules/isp/cv186x/v4l2_adapter/ 目录下面
    build_edge_pack

    # 板端卸载isp deb
    dpkg -P libisp

    # 将编译好的deb拷贝到板端，然后安装
    dpkg -i sophon-soc-libisp_xxx.deb
    ldconfig
    ```
2. 替换板端部分的isp动态库的方法
    ```shell
    # step1：先重新编译
    cd ${TOP_DIR}/middleware/v1/modules/isp/cv186x/
    export V4L2_ISP_ENABLE=1 # 边侧单独编译isp需要设置环境变量
    make
    # step2：将需要的动态库从 ${TOP_DIR}/middleware/v2/lib/ 目录下拷贝出来
    # step3: 将拷贝出来的动态库拷贝到板端，然后替换掉/opt/sophon/*isp*/lib下面的动态库
    # step4： 重新加载动态库
    ldconfig
    ```