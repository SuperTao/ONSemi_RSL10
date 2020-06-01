RSL10需要支持蓝牙OTA升级的功能。所以在固件上，分成了bootloader和FOTA两部分。

  * bootloader用于初始化板子的环境，与PC端的升级脚本进行交互，完成升级。

  * FOTA由fota.bin和app.bin打包而成。详见升级脚本mkfotaimg.py。

  * PC上运行升级脚本updater.py，通过串口，与运行在bootloader状态的RSL10交互，将FOTA部分下载到flash中。

  * OTA升级，通过android app或者PC端软件，将新的固件通过蓝牙传送给RSL10。

分析一下各部分的流程。

####  [1.RSL10资料下载网址](https://www.onsemi.cn/products/connectivity/wireless-rf-transceivers/rsl10)

####  [2.bootloader流程分析](./bootloader.md)

####  [3.升级脚本updater.py分析](./updater.md)

####  [4.FOTA打包脚本mkfotaimg.py过程](./mkfotaimg.md)

####  [5.fota升级流程](./ota升级流程分析.md)

####  [6.透传模块代码分析](./透传模块代码分析.md)
