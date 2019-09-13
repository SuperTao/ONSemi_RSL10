#!/usr/bin/env python
""" Build Tool for FOTA images.

    Prerequisites:
    - installed Python, version >=2.7 or >=3.4
"""
from __future__ import print_function


__version__ = '0.0.1'

from sys import version_info, stderr, exit

import struct
import updater as upd

if version_info < (3, ):
    from future_builtins import zip
    range = xrange
    input = raw_input
    def int_to_bytes(int, length, byteorder):
        length *= 8
        if int.bit_length() > length:
            raise OverflowError("int too big to convert")
        if byteorder == 'little':
            r = range(0, length, 8)
        elif byteorder == 'big':
            r = range(length - 8, -8, -8)
        else:
            raise ValueError("byteorder must be either 'little' or 'big'")
        return b''.join(chr((int >> p) & 0xFF) for p in r)
    def int_from_bytes(bytes, byteorder):
        length = 8 * len(bytes)
        if byteorder == 'little':
            r = range(0, length, 8)
        elif byteorder == 'big':
            r = range(length - 8, -8, -8)
        else:
            raise ValueError("byteorder must be either 'little' or 'big'")
        return sum(ord(i) << p for i, p in zip(bytes, r))
else:
    int_to_bytes = int.to_bytes
    int_from_bytes = int.from_bytes



CHIP_ID_NUM = 0x1FFFFFFC
CHIP_ID_FMT = struct.Struct("<4B")
CHIP_ID_AOBLE_PRESENT = 1
CHIP_ID_LPDSP32_PRESENT = 2
CHIP_ID_OD_PRESENT = 4

SYS_INFO_BASE = 0x00080000
SYS_INFO_FMT = struct.Struct("<2L")

SIZEOF_BONDLIST = 28
BOND_INFO_BASE = 0x00080800
BOND_INFO_FMT = struct.Struct("<B3x16sH2x6sBx16s16s8s")
BOND_INFO_STATE_EMPTY = 0xFF
BOND_INFO_STATE_INVALID = 0x00
BD_TYPE_PUBLIC = 0
BD_TYPE_PRIVATE = 1

DEV_INFO_BASE = 0x00081000
DEV_INFO_FMT = struct.Struct("<6s10x16s16s16xL16s44xH2s1820s32s32s32s")
DBG_ACCESS_LOCK = 0x4C6F634B
MANU_INFO_MAX_LENGTH = 1820

MANU_CAL_BASE = 0x00081800
MANU_CAL_FMT = struct.Struct("<2H")
MANU_ADC_FMT = struct.Struct("<2H2L")
MANU_INFO_FMT = struct.Struct("<2L")
MANU_ADDR_BASE = 0x00081A20
MANU_ADDR_FMT = struct.Struct("<6s")


def print_nvr(com):
    upd.reset(com, upd.BOOT)
    
    print("\nChip ID:");
    data = upd.do_read(com, CHIP_ID_NUM, CHIP_ID_FMT.size)
    minor, major, version, family = CHIP_ID_FMT.unpack(data)
    features = []
    if minor & CHIP_ID_AOBLE_PRESENT: features.append("AOBLE")
    if minor & CHIP_ID_LPDSP32_PRESENT: features.append("LPDSP32")
    if minor & CHIP_ID_OD_PRESENT: features.append("OD")
    if not features: features.append("(none)")
    minor >>= 3
    print("    Family          : {}".format(family))
    print("    Version         : {}".format(version))
    print("    Major Revision  : {}".format(major))
    print("    Minor Revision  : {}".format(minor))
    print("    Enabled Features: {}".format(" ".join(features)))
    
    print("\nNVR1 contents:");
    data = upd.do_read(com, SYS_INFO_BASE, SYS_INFO_FMT.size)
    start_adr, start_mem_cfg = SYS_INFO_FMT.unpack(data)
    if start_adr == 0xFFFFFFFF:
        start_adr = "(none)"
    else:
        start_adr = "0x{:08X}".format(start_adr)
    if start_mem_cfg == 0xFFFFFFFF:
        start_mem_cfg = "(none)"
    else:
        start_mem_cfg = "0x{:08X}".format(start_mem_cfg)
    print("    SYS_INFO_START_ADDR   :", start_adr)
    print("    SYS_INFO_START_MEM_CFG:", start_mem_cfg)
    
    print("\nNVR2 contents:");
    entry_size, tot_entries, num_entries = BOND_INFO_FMT.size, SIZEOF_BONDLIST, 0
    for entry, offset in enumerate(range(0, entry_size * tot_entries, entry_size)):
        data = upd.do_read(com, BOND_INFO_BASE + offset, entry_size)
        (state, ltk, ediv, addr,
         addr_type, csrk, irk, rand) = BOND_INFO_FMT.unpack(data)
        if state != BOND_INFO_STATE_EMPTY:
            if state == BOND_INFO_STATE_INVALID:
                state = "(invalid)"
            else:
                state = str(state)
            if addr_type == BD_TYPE_PUBLIC:
                addr_type = "(public)"
            elif addr_type == BD_TYPE_PRIVATE:
                addr_type = "(private)"
            else:
                addr_type = "(unknown:0x{})".format(addr_type)
            print("    Bonding Entry {}:".format(entry))
            print("        STATE                                    :", state)
            print("        LTK  (Long Term Key)                     : 0x{:032X}".format(int_from_bytes(ltk, 'little')))
            print("        EDIV (Encrypted Diversifier)             : 0x{:04X}".format(ediv))
            print("        ADDR (Peer Address)                      : 0x{:012X}".format(int_from_bytes(addr, 'little')), addr_type)
            print("        CSRK (Connection Signature Resolving Key): 0x{:032X}".format(int_from_bytes(csrk, 'little')))
            print("        IRK  (Identity Resolving Key)            : 0x{:032X}".format(int_from_bytes(irk, 'little')))
            print("        RAND (Random Key)                        : 0x{:016X}".format(int_from_bytes(rand, 'little')))
            num_entries += 1
    if num_entries == 0:
        print("    (none)")
    
    print("\nNVR3 contents:");
    data = upd.do_read(com, DEV_INFO_BASE, DEV_INFO_FMT.size)
    (addr, irk, csrk, lock_set, lock_key, manu_len, manu_ver, manu_data,
     ecdh_private, ecdh_public_x, ecdh_public_y) = DEV_INFO_FMT.unpack(data)
    if addr == b"\xFF" * 6:
        addr = "(none)"
    else:
        addr = "0x{:012X}".format(int_from_bytes(addr, 'little'))
    if irk == b"\xFF" * 16:
        irk = "(none)"
    else:
        irk = "0x{:032X}".format(int_from_bytes(irk, 'little'))
    if csrk == b"\xFF" * 16:
        csrk = "(none)"
    else:
        csrk = "0x{:032X}".format(int_from_bytes(csrk, 'little'))
    if lock_set == DBG_ACCESS_LOCK:
        lock_txt = "(locked)"
    else:
        lock_txt = "(unlocked)"
    if lock_key == b"\xFF" * 16:
        lock_key = "(none)"
    else:
        lock_key = "0x{:032X}".format(int_from_bytes(lock_key, 'little'))
    if manu_len == 0xFFFF:
        manu_len = "(none)"
        manu_ver = "(none)"
        manu_crc = "(none)"
    elif manu_len < 4 or manu_len > MANU_INFO_MAX_LENGTH:
        manu_len = "0x{:04X} (invalid)".format(manu_len)
        manu_ver = "(none)"
        manu_crc = "(none)"
    else:
        manu_code, manu_crc = struct.unpack_from("<{}sH".format(manu_len - 2), manu_data)
        manu_data = manu_ver + manu_code
        manu_len = str(manu_len)
        manu_ver = int_from_bytes(manu_ver, 'little')
        if manu_ver == 0 or manu_ver == 0xFFFF:
            manu_ver = "(none)"
        else:
            manu_ver = upd.decode_version_16bit(manu_ver)
        manu_crc = "0x{:04X}".format(manu_crc)
    if ecdh_private == b"\xFF" * 32:
        ecdh_private = "(none)"
    else:
        ecdh_private = "0x{:064X}".format(int_from_bytes(ecdh_private, 'little'))
    if ecdh_public_x == b"\xFF" * 32:
        ecdh_public_x = "(none)"
    else:
        ecdh_public_x = "0x{:064X}".format(int_from_bytes(ecdh_public_x, 'little'))
    if ecdh_public_y == b"\xFF" * 32:
        ecdh_public_y = "(none)"
    else:
        ecdh_public_y = "0x{:064X}".format(int_from_bytes(ecdh_public_y, 'little'))
    print("    DEVICE_INFO_BLUETOOTH_ADDR:", addr)
    print("    DEVICE_INFO_BLUETOOTH_IRK :", irk)
    print("    DEVICE_INFO_BLUETOOTH_CSRK:", csrk)
    print("    LOCK_INFO_SETTING         : 0x{:08X} {}".format(lock_set, lock_txt))
    print("    LOCK_INFO_KEY             :", lock_key)
    print("    MANU_INFO_LENGTH          :", manu_len)
    print("    MANU_INFO_FUNCTION_VERSION:", manu_ver)
    print("    MANU_INFO_FUNCTION_CODE   :", "-")
    print("    MANU_INFO_FUNCTION_CRC    :", manu_crc)
    print("    DEVICE_INFO_ECDH_PRIVATE  :", ecdh_private)
    print("    DEVICE_INFO_ECDH_PUBLIC_X :", ecdh_public_x)
    print("    DEVICE_INFO_ECDH_PUBLIC_Y :", ecdh_public_y)
    
    print("\nNVR4 contents:");
    data = upd.do_read(com, MANU_CAL_BASE, 256)
    start, entry_size, tot_entries = 0, MANU_CAL_FMT.size, 4
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_BANDGAP      @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VDDRF        @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VDDPA        @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VDDC         @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VDDC_STANDBY @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VDDM         @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VDDM_STANDBY @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFF > target > 0:
            print("    MANU_INFO_VCC          @{:6}mV : 0x{:04X}".format(10 * target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFFFF > target > 0:
            print("    MANU_INFO_OSC_32K      @{:6}Hz : 0x{:04X}".format(target, trim))
    start += entry_size * tot_entries
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFFFF > target > 0:
            print("    MANU_INFO_OSC_RC       @{:6}kHz: 0x{:04X}".format(target, trim))
    start += entry_size * tot_entries * 2
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        trim, target = MANU_CAL_FMT.unpack_from(data, offset)
        if 0xFFFF > target > 0:
            print("    MANU_INFO_OSC_RC_MULT  @{:6}kHz: 0x{:04X}".format(target, trim))
    start += entry_size * tot_entries
    entry_size, tot_entries, levels = MANU_ADC_FMT.size, 4, (1200, 1120, 1150, 1250)
    for entry, offset in enumerate(range(start, start + entry_size * tot_entries, entry_size)):
        offset_lo, offset_hi, gain_lo, gain_hi = MANU_ADC_FMT.unpack_from(data, offset)
        if (0xFFFF  > offset_lo > 0 and 0xFFFF  > offset_hi > 0 and 
            0x1FFFF > gain_lo   > 0 and 0x1FFFF > gain_hi   > 0):
            print("    MANU_INFO_ADC          @{:6}mV : 0x{:04X} / 0x{:04X} / 0x{:05X} / 0x{:05X} /".format(levels[entry], offset_lo, offset_hi, gain_lo, gain_hi))
    version, crc = MANU_INFO_FMT.unpack(data[-MANU_INFO_FMT.size:])
    if version == 0xFFFFFFFF:
        version = "(none)"
    else:
        version = "0x{:08X}".format(version)
    print("    MANU_INFO_VERSION                :", version)
    print("    MANU_INFO_CRC                    : 0x{:08X}".format(crc))

    data = upd.do_read(com, MANU_ADDR_BASE, MANU_ADDR_FMT.size)
    addr,  = MANU_ADDR_FMT.unpack(data)
    if addr == b"\xFF" * 6:
        addr = "(none)"
    else:
        addr = "0x{:012X}".format(int_from_bytes(addr, 'little'))
    print("    MANU_INFO_BLUETOOTH_ADDR         :", addr)
    
    #upd.do_restart(com)


if __name__ == "__main__":
    
    import argparse
    import sys
    
    #sys.tracebacklimit = 0
    
    parser = argparse.ArgumentParser(description='Reads or writes NVR content.')
    parser.add_argument('--version', action='version', version="%(prog)s " + __version__)
    parser.add_argument('port', metavar='PORT', type=str,
                        help="COM port of the RSL10 UART")
    args = parser.parse_args()
    
    with upd.ComPort(args.port) as com:
        print_nvr(com)
