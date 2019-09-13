RSL10需要支持OTA升级的功能。所以在固件上，分成了bootloader和FOTA两部分。

* bootloader用于初始化板子的环境。

* FOTA由fota.bin和app.bin打包而成。

* PC上运行升级脚本，通过串口，将FOTA部分下载到RSL10中。

* OTA升级，通过android app或者PC短软件，将新的固件通过蓝牙传送给RSL10。

分析一下各部分的流程。

### 1. [RSL10资料下载网址](https://www.onsemi.cn/products/connectivity/wireless-rf-transceivers/rsl10)

### 2. [bootloader流程分析](./bootloader.md)

### 3. [升级脚本分析](./updater.md)

### 4. [FOTA打包过程](./mkfotaimg.md)

### 5. [fota升级流程](./ota升级流程分析.md)

### 6. [透传模块代码分析](./透传模块代码分析.md)
