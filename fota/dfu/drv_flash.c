/* ----------------------------------------------------------------------------
 * Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * drv_flash.c
 * - Implementation of the flash driver.
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <rsl10.h>
#include <rsl10_flash_rom.h>

#include "drv_flash.h"

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Flash_Unlock(void)
 * ----------------------------------------------------------------------------
 * Description   : Unlocks the flash for programming/erasing.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Flash_Unlock(void)
{
    FLASH->MAIN_CTRL = MAIN_LOW_W_ENABLE    |
                       MAIN_MIDDLE_W_ENABLE |
                       MAIN_HIGH_W_ENABLE;
    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Flash_Lock(void)
 * ----------------------------------------------------------------------------
 * Description   : Locks the flash for programming/erasing.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Flash_Lock(void)
{
    FLASH->MAIN_CTRL = 0;
    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;
}

/* ----------------------------------------------------------------------------
 * Function      : bool Drv_Flash_Program(uint_fast32_t  adr,
 *                                        const uint32_t data_a[])
 * ----------------------------------------------------------------------------
 * Description   : Programs a double word to the flash memory.
 * Inputs        : adr              - start address of the 1st word
 * Inputs        : data_a           - double word to program
 * Outputs       : return value     - true  success
 *                                  - false failure
 * Assumptions   :
 * ------------------------------------------------------------------------- */
bool Drv_Flash_Program(uint_fast32_t adr, const uint32_t data_a[])
{
    return (Flash_WriteWordPair(adr,
                                data_a[0],
                                data_a[1]) == FLASH_ERR_NONE &&
            memcmp((void *)adr, data_a, 2 * sizeof(uint32_t)) == 0);
}

/* ----------------------------------------------------------------------------
 * Function      : bool Drv_Targ_Erase(uint_fast32_t  adr)
 * ----------------------------------------------------------------------------
 * Description   : Erases a flash sector.
 * Inputs        : adr              - start address of the sector
 * Outputs       : return value     - true  success
 *                                  - false failure
 * Assumptions   :
 * ------------------------------------------------------------------------- */
bool Drv_Flash_Erase(uint_fast32_t adr)
{
    return (Flash_EraseSector(adr) == FLASH_ERR_NONE);
}
