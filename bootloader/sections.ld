/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
 * ----------------------------------------------------------------------------
 * sections.ld
 * - Linker script for BootLoader with relocator to PRAM
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Memory linker description
 * ------------------------------------------------------------------------- */
MEMORY
{
  ROM  (r) :        ORIGIN = 0x00000000, LENGTH = 4K
  FLASH (xrw) :     ORIGIN = 0x00100000, LENGTH = 8K
  PRAM (xrw) :      ORIGIN = 0x00200000, LENGTH = 32K

  DRAM (xrw) :      ORIGIN = 0x20000000, LENGTH = 24K
  DRAM_DSP (xrw) :  ORIGIN = 0x20006000, LENGTH = 48K
  DRAM_BB (xrw) :   ORIGIN = 0x20012000, LENGTH = 16K
}

/* ----------------------------------------------------------------------------
 * Stack related defines and provided variables
 * ------------------------------------------------------------------------- */
__stack = ORIGIN(DRAM) + LENGTH(DRAM);

PROVIDE ( __stack = __stack );

/*
 * Default stack sizes.
 * These are used by the startup in order to allocate stacks 
 * for the different modes.
 */

__Main_Stack_Size = 1024 ;
PROVIDE ( _Main_Stack_Size = __Main_Stack_Size );

__Main_Stack_Limit = __stack  - __Main_Stack_Size;
PROVIDE ( _Main_Stack_Limit = __Main_Stack_Limit );

/* ----------------------------------------------------------------------------
 * Heap related defines and provided variables
 * ------------------------------------------------------------------------- */
PROVIDE ( __Heap_Begin__ = __noinit_end__ );
PROVIDE ( __Heap_Limit__ = __stack - __Main_Stack_Size );

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

    /* 
     * This address is used by the startup code to 
     * initialise the .text section in PRAM from BOOT.
     */
    __text_init__ = .;
    
    .text  : AT ( __text_init__ )
    {
        __text_start__ = .;
        KEEP(*(.interrupt_vector))

        /* 
         * This section is here to store the startup code immediately after
         * the interrupt vectors, as required by the program ROM.
         */
        *(.reset)

        /* Pre-initialization Code */
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start__ = .);

        /* System initialization and the platform initialization (if present)
         * should be first */
        KEEP(*(.preinit_array_sysinit .preinit_array_sysinit.*))
        KEEP(*(.preinit_array_platform .preinit_array_platform.*))

        /* Pre-initialization functions (to be executed before C++ 
         * constructors are run) */
        KEEP(*(.preinit_array .preinit_array.*))

        PROVIDE_HIDDEN (__preinit_array_end__ = .);

        /* Initialization Code */
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start__ = .);

        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))

        PROVIDE_HIDDEN (__init_array_end__ = .);

        /*
         * The program code is stored in the .text section, 
         * which goes to FLASH.
         */
        . = ALIGN(4);
   
        *(.text .text.*)            /* all remaining code */
        *(.rodata .rodata.*)        /* read-only data (constants) */
        
    } >PRAM

    /* Forces the placement of implicitly generated code, like .ARM.exidx*,
     * so that no overlap with the following data segments occure. */
    .dummy  :
    {
        /* 
         * This address is used by the startup code to 
         * initialise the .text section in PRAM from BOOT.
         */
        . = ALIGN(4);
        __text_end__ = .;
        
    } >PRAM
    __text_size__ = __text_end__ - __text_start__;
    
    /* Place the SystemClock variable needed for CMSIS in a place that is
     * compatible with the ROM's placement of this variable so that the 
     * variable can be used by CMSIS and the ROM's flash write libary */
    .systemclock (NOLOAD) :
    {
        KEEP(*(.systemclock))
        
        . = ALIGN(4);
    } >DRAM
    
    /* 
     * This address is used by the startup code to 
     * initialise the .data section in DRAM from BOOT.
     */
    __data_init__ = __text_init__ + __text_size__;
    
    /*
     * The initialised data section.
     * The program executes knowing that the data is in the RAM
     * but the loader puts the initial values in the FLASH (inidata).
     * It is one task of the startup to copy the initial values from 
     * FLASH to RAM.
     */
    .data  : AT ( __data_init__ )
    {
        /* This is used by the startup code to initialise the .data section */
        __data_start__ = . ;
        
        *(.data_begin .data_begin.*)
        *(.data .data.*)
        *(.data_end .data_end.*)
        
        /* This is used by the startup code to initialise the .data section */
        . = ALIGN(4);
        __data_end__ = . ;
        
    } >DRAM
    __data_size__ = __data_end__ - __data_start__;
    
    /*
     * The uninitialised data section. NOLOAD is used to avoid
     * the "section `.bss' type changed to PROGBITS" warning
     */
    .bss (NOLOAD) :
    {
        __bss_start__ = .;
        
        *(.bss_begin .bss_begin.*)
        *(.bss .bss.*)
        *(COMMON)
        *(.bss_end .bss_end.*)
        
        . = ALIGN(4);
        __bss_end__ = .;
        
    } >DRAM
    __bss_size__ = __bss_end__ - __bss_start__;
    
    .noinit (NOLOAD) :
    {
        __noinit_start__ = .;
        
        *(.noinit .noinit.*) 
        
         . = ALIGN(4) ;
        __noinit_end__ = .;   
        
    } >DRAM
    __noinit_size__ = __noinit_end__ - __noinit_start__;
    
    . = __data_init__ + __data_size__;
    __flash_end__ = ALIGN(0x800);
    
}
