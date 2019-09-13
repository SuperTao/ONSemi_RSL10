RSL10需要支持OTA升级的功能。所以在固件上，分成了bootloader和FOTA两部分。

bootloader用于初始化板子的环境。

FOTA又有fota.bin和app.bin打包而成。

PC上运行升级脚本，通过串口，将FOTA部分下载到RSL10中。

分析一下各部分的流程。

### [bootloader流程分析](./bootloader.md)

### 升级脚本分析(./updater.md)

### FOTA打包过程(./mkfotaimg.md)

### fota升级流程(./ota升级流程分析.md)

### 透传模块代码分析(./透传模块代码分析.md)
