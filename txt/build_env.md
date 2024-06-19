1.升级gcc 
    【gcc4.9.2安装方式：sudo yum install devtoolset-3-toolchain，执行：source       
     /opt/rh/devtoolset-3/enable 切换gcc】
2. centos 安装软件
    yum install -y 软件名
    rpm -i  安装包名称
    源码包编译安装软件，编译生成二进制文件

    -yum install -y 软件名
     yum update -y  软件名
     yum remove -y  软件名

     yum list installed 查找安装过的包

    如果没有想要的软件包，可以切yum源

    rpm安装
        rpm是以一种数据库记录的方式来将所需要的套件安装在Linux主机的一套管理程序，也就是说Linux系统中存在一个关于rpm的数据库，
        它记录了安装的包与包之间的依赖相关性。rpm包是预先在Linux主机上编译好并打包的二进制文件，省去了下面介绍的源码包安装的编译等过程，
        安装起来非常快捷。

        rpm -ivh  v: 显示正在安装的文件信息， h : 安装进度
        