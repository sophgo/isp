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

## for developer
​	在/opt/sophon/目录下面带有dev后缀的isp目录供开发者使用。在安装dev debain包前，确保runtime的isp debain包已经安装。

1. 目录结构
(1) data
    放置不同sensor的pqbin以及环境变量的配置文件，环境配置文件可以按需拷贝到/etc/profile.d/以及etc/ld.so.conf.d/目录下面。
(2) lib
    放置src目录下面编译生成的动态库。刚安装的时候默认不放置动态库。
(3) bin
    放置可执行文件以及执行文件需要的一些配置文件。
(4) doc
    放置文档。
(5) include
    放置src目录下面的c文件所依赖的头文件，开发者的应用只需包含cvi_isp_v4l2.h即可，一般可不关注inc下面的头文件。
(6) sample
    应用例子。开发者可参数该应用样例开发自己的应用。
(7) src
    源文件，可根据开发需求进行修改，比如修改sensor类型，然后编译，生成的动态库放在lib目录下面，供sample使用。

2. 开发流程
(1) 进入src目录下，可修改.config来选择打开某些sensor，然后make，会生成libispv4l2_helper_dev.so和libsns_full_dev.so，这两个库会放置在lib目录下。
(2) 进入到sample目录下，make，然后会生成isp-tool-daemon可执行文件。然后执行：export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH; ldconfig 来更新加载动态库cache，然后执行./CviIspTool.sh即可启动程序。
