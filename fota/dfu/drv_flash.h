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
 * drv_flash.h
 * - Interface to the Flash driver.
 * ------------------------------------------------------------------------- */

#ifndef _DRV_FLASH_H    /* avoids multiple inclusion */
#define _DRV_FLASH_H

#include <stdbool.h>
#include <stdint.h>

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Flash_Unlock(void)
 * ----------------------------------------------------------------------------
 * Description   : Unlocks the flash for programming/erasing.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Flash_Unlock(void);

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Flash_Lock(void)
 * ----------------------------------------------------------------------------
 * Description   : Locks the flash for programming/erasing.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Flash_Lock(void);

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
bool Drv_Flash_Program(uint_fast32_t adr, const uint32_t data_a[]);

/* ----------------------------------------------------------------------------
 * Function      : bool Drv_Targ_Erase(uint_fast32_t  adr)
 * ----------------------------------------------------------------------------
 * Description   : Erases a flash sector.
 * Inputs        : adr              - start address of the sector
 * Outputs       : return value     - true  success
 *                                  - false failure
 * Assumptions   :
 * ------------------------------------------------------------------------- */
bool Drv_Flash_Erase(uint_fast32_t adr);

#endif    /* _DRV_FLASH_H */
