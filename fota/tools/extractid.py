#!/usr/bin/env python

import struct

FLASH_START = 0x100000
FLASH_SIZE  = 380 * 1024
RAM_START   = 0x20000000
RAM_SIZE    = 88 * 1024

BOOT_BASE_ADR = FLASH_START
BOOT_MAX_SIZE = 8 * 1024
APP_BASE_ADR  = BOOT_BASE_ADR + BOOT_MAX_SIZE
APP_MAX_SIZE  = FLASH_SIZE - BOOT_MAX_SIZE

def eval_header(img):
    def check_bounds(low, adr, high, text, align=(2, 1)):
        assert adr % align[0] == align[1] and low <= adr < high, text + " (0x{0:08X})".format(adr)
    # unpack image header
    (stack_ptr, reset_handler,
    nmi_handler, hardfault_handler,
    memmanage_handler, busfault_handler,
    usagefault_hander, version_ptr, dscr_ptr) = struct.unpack_from("<9L", img)
    # image must start at flash sector boundary (which is 2KiB)
    # and the reset handler is in the 1st sector
    img_start = reset_handler - reset_handler % (2 * 1024)
    img_size = len(img)
    # check image validity
    check_bounds(RAM_START + 1024, stack_ptr, RAM_START + RAM_SIZE, "Stack pointer invalid", align=(4, 0))
    check_bounds(img_start + 8 * 4, reset_handler, img_start + 90 * 4, "Reset vector invalid")
    check_bounds(reset_handler, nmi_handler, img_start + img_size, "NMI vector invalid")
    check_bounds(reset_handler, hardfault_handler, img_start + img_size, "HardFault vector invalid")
    check_bounds(reset_handler, memmanage_handler, img_start + img_size, "MemManage vector invalid")
    check_bounds(reset_handler, busfault_handler, img_start + img_size, "BusFault vector invalid")
    check_bounds(reset_handler, usagefault_hander, img_start + img_size, "UsageFault vector invalid")
    check_bounds(reset_handler, version_ptr, img_start + img_size, "Version pointer invalid", align=(2, 0))
    check_bounds(reset_handler, dscr_ptr, img_start + img_size, "Descriptor pointer invalid", align=(4, 0))
    return img_start, img_size, dscr_ptr

def load_image(name):
    with open(name, 'rb') as f:
        img = f.read()
    img_start, img_size, dscr_ptr = eval_header(img)
    assert img_start ==  APP_BASE_ADR, "Image start address invalid (0x{0:08X})".format(img_start)
    return img, dscr_ptr - img_start

def extract_id(name):
    img, dscr_offset = load_image(name)
    id = img[dscr_offset+4:dscr_offset+36]
    return id


if __name__ == "__main__":
    
    import sys
    import os.path
    
    input = sys.argv[1]
    output = os.path.splitext(input)[0] + ".id"
    with open(output, 'wb') as out:
        out.write(extract_id(input))
