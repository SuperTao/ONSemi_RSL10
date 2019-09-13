程序启动，根据连接脚本确定入口函数。

sections.ld内容如下
```
/* 
 * The entry point is informative, for debuggers and simulators,
 * since the Cortex-M vector points to it anyway.
 */
ENTRY(Reset_Handler)


/* Sections Definitions */

SECTIONS
{
    /*
     * For Cortex-M devices, the beginning of the startup code is stored in
     * the .boot_vector section, which goes to FLASH 
     */
    .boot  :
    {
        KEEP(*(.boot_vector))

        /* 
         * This section places the rersident part of the BootLoader into
         * the flash memory.
         */
        *sys_boot.o(*.Sys_Boot_ResetHandler)
        *sys_boot.o(*.Sys_Boot_version)
        *sys_boot.o(.text .text.* .rodata .rodata.*)
        
        . = ALIGN(4);
        
    } >FLASH
```

这一部分按理说应该是从ENTRY指定的Reset_Handler开始运行，但是实际情况是，从.boot_vector段开始运行。

.boot_vector定义位于startup.S中。如下：

```
/* ----------------------------------------------------------------------------
 * Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
 * Semiconductor). All Rights Reserved.
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 * ----------------------------------------------------------------------------
 * startup.S
 * - GCC compatible CMSIS Cortex-M3 Core Device start-up file for the
 *   ARM Cortex-M3 processor
 *   从这段注释可以看出，有两个向量表，一个用于启动，一个用于升级
 * - It has two vector tables, one for the Boot-Up and the other for FW update
 *   process. The later is relocated to PRAM by the linker.
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Boot Vector Table
 * ------------------------------------------------------------------------- */
	// 把这部分代码放到.boot_vector段
    .section ".boot_vector"
    // 全局函数或者对象
    .globl  BootVector_Table
    // 定义一个BootVector_Table对象
    .type   BootVector_Table, %object

// BootVector_Table初始化
BootVector_Table:
	// 定义长整型变量__stack，占用4字节
    .long   __stack                         /*  0 Initial Stack Pointer */
    .long   Sys_Boot_ResetHandler           /*  1 Reset Handler */
    .long   Sys_Boot_ResetHandler           /*  2 Non-Maskable Interrupt Handler */
    .long   Sys_Boot_ResetHandler           /*  3 Hard Fault Handler */
    .long   Sys_Boot_ResetHandler           /*  4 Mem Manage Fault Handler */
    .long   Sys_Boot_ResetHandler           /*  5 Bus Fault Handler */
    .long   Sys_Boot_ResetHandler           /*  6 Usage Fault Handler */
    .long   Sys_Boot_version                /*  7 Pointer to version info */
    .long   Sys_Boot_Updater                /*  8 Entry point for Updater */
    .long   Sys_Boot_NextImage              /*  9 Sys_Boot_GetNextImage */
    // 计算BootVector_Table的大小，"."当前地址，减去BootVector_Table的首地址
    .size   BootVector_Table, . - BootVector_Table



/* ----------------------------------------------------------------------------
 * RAM Vector Table
 * ------------------------------------------------------------------------- */
	// 定义.interrupt_vector段
    .section ".interrupt_vector"
    .globl  ISR_Vector_Table
    .type   ISR_Vector_Table, %object


ISR_Vector_Table:
    .long   __stack                         /*  0 Initial Stack Pointer */
    .long   Reset_Handler                   /*  1 Reset Handler */
    .long   NMI_Handler                     /*  2 Non-Maskable Interrupt Handler */
    .long   HardFault_Handler               /*  3 Hard Fault Handler */
    .long   MemManage_Handler               /*  4 Mem Manage Fault Handler */
    .long   BusFault_Handler                /*  5 Bus Fault Handler */
    .long   UsageFault_Handler              /*  6 Usage Fault Handler */
    .long   Sys_Boot_version                /*  7 Pointer to version info */
    .long   0                               /*  8 Reserved */
    .long   0                               /*  9 Reserved */
    .long   0                               /* 10 Reserved */
    .long   SVC_Handler                     /* 11 Supervisor Call Handler */
    .long   DebugMon_Handler                /* 12 Debug Monitor Handler */
    .long   0                               /* 13 Reserved */
    .long   PendSV_Handler                  /* 14 PendSV Handler */
    .long   SysTick_Handler                 /* 15 SysTick Handler */
    .size   ISR_Vector_Table, . - ISR_Vector_Table

    .thumb

/* ----------------------------------------------------------------------------
 * Reset Handler
 * ------------------------------------------------------------------------- */

    .section .reset,"x",%progbits
    .thumb_func
    // 定义全局函数，可以被其他文件中的函数使用
    .globl  Reset_Handler
    // .type表示Reset_Handler是一个function(函数)
    .type   Reset_Handler, %function
    // 函数定义
Reset_Handler:
	// 函数开始标志
    .fnstart
    // 将SystemInit的地址保存到R0中，这个SystemInit的定义应该是在libcmsis.a库函数中
    // 可以参考ONSemiconductor/RSL10/3.0.534/source/firmware/cmsis/source/system_rsl10.c中的函数定义
    LDR     R0, =SystemInit
    // 跳转到R0的寄存器地址的位置SystemInit，将处理器的工作状态有ARM 状态切换到Thumb 状态
    BLX     R0
    // _start地址保存到R0中，_start也是在库函数中定义
    // ONSemiconductor/RSL10/3.0.534/source/firmware/cmsis/source/start.c中定义
    // _start函数，最后调用main()函数
    LDR     R0, =_start
    // BX 指令跳转到指令中所指定的目标地址_start，目标地址处的指令既可以是ARM 指令，也可以是Thumb指令。
    BX      R0
    // 用于声明一个数据缓冲池
    .pool
    // 防止通过当前函数展开。不需要或不允许个性例程或异常表数据
    .cantunwind
    .fnend
    // 设置Reset_Handler函数的大小，"."表示当前地址，减去Reset_Handler的地址，就是函数的大小
    .size   Reset_Handler, . - Reset_Handler

    .section ".text"

/* ----------------------------------------------------------------------------
 * Place-holder Exception Handlers
 * - Weak definitions
 * - If not modified or replaced, these handlers implement infinite loops
 * ------------------------------------------------------------------------- */
    // 申明NMI_Handler是一个弱符号，如果其他地方定义了，那么这个文件的定义将会被覆盖
    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
	// 跳转到当前地址，就是一个死循环
    B       .
    .size   NMI_Handler, . - NMI_Handler

    .weak   HardFault_Handler
    .type   HardFault_Handler, %function
HardFault_Handler:
    B       .
    .size   HardFault_Handler, . - HardFault_Handler

    .weak   MemManage_Handler
    .type   MemManage_Handler, %function
MemManage_Handler:
    B       .
    .size   MemManage_Handler, . - MemManage_Handler

    .weak   BusFault_Handler
    .type   BusFault_Handler, %function
BusFault_Handler:
    B       .
    .size   BusFault_Handler, . - BusFault_Handler

    .weak   UsageFault_Handler
    .type   UsageFault_Handler, %function
UsageFault_Handler:
    B       .
    .size   UsageFault_Handler, . - UsageFault_Handler

    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    B       .
    .size   SVC_Handler, . - SVC_Handler

    .weak   DebugMon_Handler
    .type   DebugMon_Handler, %function
DebugMon_Handler:
    B       .
    .size   DebugMon_Handler, . - DebugMon_Handler

    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    B       .
    .size   PendSV_Handler, . - PendSV_Handler

    .weak   SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    B       .
    .size   SysTick_Handler, . - SysTick_Handler

    .end
```

.boot_vector段跳转到Sys_Boot_ResetHandler运行

定义位于sys_boot.c
```
void Sys_Boot_ResetHandler(void)
{
    Init();
    // IO引脚电平，用来判断是否需要更新程序
    if (!CheckUpdatePin())
    {
    	// 判断是否有镜像需要拷贝，以及拷贝操作是否成功。
        if (CopyImage(ValidateImage()))
        {
        	// 启动app
            StartApp();	
        }

        /* Fall through to Updater, if Application start failed */
    }
    // 启动失败，进入更新程序
    Sys_Boot_Updater();
}
```

启动app部分不分析，分析更新程序的部分。

```
void Sys_Boot_Updater(void)
{
    /* Initialize system */
    Sys_Initialize();

    /* Start Updater from PRAM */
    Sys_BootROM_StartApp(LoadUpdaterCode()); // 跳转到.interrupt_vector段

    /* If Updater start failed -> wait for Reset */
    for (;;);
}
```

LoadUpdaterCode获取地址

```
static uint32_t * LoadUpdaterCode(void)
{
	// 这几个变量都是定义在sections.ld中
    extern uint32_t __text_init__;
    extern uint32_t __text_start__;
    extern uint32_t __text_end__;
    // C语言引用链接脚本中的变量，不能直接赋值，要采用如下的方式进行引用
    uint32_t *src_p = &__text_init__;
    uint32_t *dst_p = &__text_start__;

    /* Copy the Updater code from Flash to PRAM */
    while (dst_p < &__text_end__)
    {
        *dst_p++ = *src_p++;
    }

    return &__text_start__;
}
```

Sys_BootROM_StartApp从指定的地址获取函数，然后把.interrupt_vector段首地址作为参数传入。

```
__STATIC_INLINE BootROMStatus Sys_BootROM_StartApp(uint32_t* vect_table)
{
	// 从地址0x00000034获取函数Sys_BootROM_StartApp,参数是传入的参数
    return (*((BootROMStatus (**)(uint32_t*))
              ROMVECT_BOOTROM_START_APP))(vect_table);
}
```

startup.S中定义如下：

```
    .section ".interrupt_vector"
    .globl  ISR_Vector_Table
    .type   ISR_Vector_Table, %object


ISR_Vector_Table:
    .long   __stack                         /*  0 Initial Stack Pointer */
    .long   Reset_Handler                   /*  1 Reset Handler */
    .long   NMI_Handler                     /*  2 Non-Maskable Interrupt Handler */
    .long   HardFault_Handler               /*  3 Hard Fault Handler */
    .long   MemManage_Handler               /*  4 Mem Manage Fault Handler */
    .long   BusFault_Handler                /*  5 Bus Fault Handler */
    .long   UsageFault_Handler              /*  6 Usage Fault Handler */
    .long   Sys_Boot_version                /*  7 Pointer to version info */
    .long   0                               /*  8 Reserved */
    .long   0                               /*  9 Reserved */
    .long   0                               /* 10 Reserved */
    .long   SVC_Handler                     /* 11 Supervisor Call Handler */
    .long   DebugMon_Handler                /* 12 Debug Monitor Handler */
    .long   0                               /* 13 Reserved */
    .long   PendSV_Handler                  /* 14 PendSV Handler */
    .long   SysTick_Handler                 /* 15 SysTick Handler */
    .size   ISR_Vector_Table, . - ISR_Vector_Table

    .thumb
```

所以跳转到Reset_Handler函数执行。

```
    .section .reset,"x",%progbits
    .thumb_func
    // 定义全局函数，可以被其他文件中的函数使用
    .globl  Reset_Handler
    // .type表示Reset_Handler是一个function(函数)
    .type   Reset_Handler, %function
    // 函数定义
Reset_Handler:
	// 函数开始标志
    .fnstart
    // 将SystemInit的地址保存到R0中，这个SystemInit的定义应该是在libcmsis.a库函数中
    // 可以参考ONSemiconductor/RSL10/3.0.534/source/firmware/cmsis/source/system_rsl10.c中的函数定义
    LDR     R0, =SystemInit
    // 跳转到R0的寄存器地址的位置SystemInit，将处理器的工作状态有ARM 状态切换到Thumb 状态
    BLX     R0
    // _start地址保存到R0中，_start也是在库函数中定义
    // ONSemiconductor/RSL10/3.0.534/source/firmware/cmsis/source/start.c中定义
    // _start函数，最后调用main()函数
    LDR     R0, =_start
    // BX 指令跳转到指令中所指定的目标地址_start，目标地址处的指令既可以是ARM 指令，也可以是Thumb指令。
    BX      R0
    // 用于声明一个数据缓冲池
    .pool
    // 防止通过当前函数展开。不需要或不允许个性例程或异常表数据
    .cantunwind
    .fnend
    // 设置Reset_Handler函数的大小，"."表示当前地址，减去Reset_Handler的地址，就是函数的大小
    .size   Reset_Handler, . - Reset_Handler

    .section ".text"
```

最后跳转到main(), sys_upd.c

```
int main(void)
{
	// 时钟，串口初始化
    Init();

    for (;;)
    {
        /* Wait for CMD message */
        ProcessCmd();
        // 喂狗，防止复位
        /* Feed Watchdog */
        Drv_Targ_Poll();
    }

    /* never reached */
    return 0;
}
```

接受串口发送过来的命令。

```
static void ProcessCmd(void)
{
	// 接受命令
    cmd_msg_t  *cmd_p = RecvCmd();

    switch (cmd_p->type)
    {
		// 处理HELLO命令
        case HELLO:
        {
            ProcessHello();
        }
        break;
		// 正常传输串口数据
        case PROG:
        {
            ProcessProg(&cmd_p->arg.prog);
        }
        break;

    #if (CFG_READ_SUPPORT)
        case READ:
        {
            ProcessRead(cmd_p);
        }
        break;
    #endif /* if (CFG_READ_SUPPORT) */
		// 发送完成，进行重启
        case RESTART:
        {
            ProcessRestart();
        }
        break;

        default:
        {
            SendError(UNKNOWN_CMD);
        }
        break;
    }
}
```

其中RecvCmd()函数中的定义超时时间30s

```
static cmd_msg_t * RecvCmd(void)
{
    cmd_msg_t    *cmd_p;
#if (CFG_TIMEOUT > 0)
    uint_fast32_t start_tick = Drv_Targ_GetTicks();
#endif
    do
    {
        /* Feed the watchdag */
        Drv_Targ_Poll();

        /* Receive command */
        Drv_Uart_StartRecv(sizeof(*cmd_p));
        cmd_p = Drv_Uart_FinishRecv();

#if (CFG_TIMEOUT > 0)
        // 计算超时时间，超过这个时间，就进行复位
        /* Check timeout */
        if (Drv_Targ_GetTicks() - start_tick > CFG_TIMEOUT * 1000)
        {
            Drv_Targ_Reset();
        }
#endif
    }
    while (cmd_p == NULL);

    return cmd_p;
}
```

串口的数据是先发送HELLO,然后再发送各种数据，最后发送重启命令。

从其有运行开始的StartApp();	函数，进行正常的开机流程。

串口发送是有updater.py脚本发送的，后面会分析这个脚本。