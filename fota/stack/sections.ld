EXTERN(Kernel_Init Kernel_Schedule);
EXTERN(BLE_Init BLE_InitNoTL BLE_Reset);
/*
SECTIONS
{
    .text_stack :
    {
        KEEP(*.o(.text .text.*))
        KEEP(*.o(.rodata .rodata.*))
    }
    .data_stack :
    {
        *.o(.data .data.*)
    }
    .bss_stack :
    {
        *.o(.bss .bss.* COMMON)
    }
    .noinit_stack :
    {
        *.o(.noinit .noinit.*)
    }
}
*/
