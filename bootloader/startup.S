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
 *
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

