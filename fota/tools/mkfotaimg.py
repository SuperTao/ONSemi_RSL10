#!/usr/bin/env python
""" Build Tool for FOTA images.

    Prerequisites:
    - installed Python, version >=2.7 or >=3.4
    - installed module ecdsa, version >=0.13
"""
from __future__ import print_function


__version__ = '1.0.0'

from sys import version_info, stderr, exit

import hashlib
import struct

try:
    import ecdsa
except ImportError:
    print("The package 'ecdsa' is not installed! Please install it with 'pip install ecdsa'.", file=stderr)
    exit(1)


if version_info < (3, ):
    from future_builtins import zip
    range = xrange
    input = raw_input
    octets = bytearray
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
    octets = lambda x: x
    int_to_bytes = int.to_bytes
    int_from_bytes = int.from_bytes


CURVE = ecdsa.curves.NIST256p
HASHFUNC = hashlib.sha256

# The default encoding of ecdsa is big-endian, but the micro-ecc
# uses little-endian, so here are encoding/decoding routines
# for little-endian.

def sigencode_string(r, s, order):
    l = (order.bit_length() + 7) // 8
    r_str = int_to_bytes(r, l, 'little')
    s_str = int_to_bytes(s, l, 'little')
    return r_str + s_str

def sigdecode_string(signature, order):
    l = (order.bit_length() + 7) // 8
    assert len(signature) == 2*l, (len(signature), 2*l)
    r = int_from_bytes(signature[:l], 'little')
    s = int_from_bytes(signature[l:], 'little')
    return r, s

def vkeyrecode_string(vkey_str):
    l = len(vkey_str) // 2 - 1
    x_str = vkey_str[l::-1]
    y_str = vkey_str[:l:-1]
    return x_str + y_str


FLASH_START = 0x100000
FLASH_SIZE  = 380 * 1024
FLASH_SECTOR_SIZE = 2 * 1024
RAM_START   = 0x20000000
RAM_SIZE    = 88 * 1024

BOOT_BASE_ADR = FLASH_START
BOOT_MAX_SIZE = 8 * 1024
FOTA_BASE_ADR = BOOT_BASE_ADR + BOOT_MAX_SIZE
FOTA_MAX_SIZE = (FLASH_SIZE - BOOT_MAX_SIZE) // 2

IMG_HDR_FMT  = struct.Struct("<7L2L")
IMG_DSCR_FMT = struct.Struct("<L32s")
IMG_VER_FMT  = struct.Struct("<6sH")
IMG_DEV_FMT  = struct.Struct("<16s")
IMG_CFG_FMT  = struct.Struct("<L64s16sH29s")


def check_img(imgfile, start_adr, max_size):
    def check_bounds(low, adr, high, text, align=(2, 1)):
        assert adr % align[0] == align[1] and low <= adr < high, text + " (0x{0:08X})".format(adr)
    
    img = imgfile.read()
    # unpack image header
    (stack_ptr, reset_handler,
    nmi_handler, hardfault_handler,
    memmanage_handler, busfault_handler,
    usagefault_hander, version_ptr, dscr_ptr) = IMG_HDR_FMT.unpack_from(img)
    # image must start at flash sector boundary (which is 2KiB)
    # and the reset handler is in the 1st sector
    img_start = reset_handler - reset_handler % (2 * 1024)
    img_size = len(img)
    
    # check image validity
    assert img_start == start_adr, "Wrong image start address (0x{0:08X}/0x{1:08X})".format(img_start, start_adr)
    assert img_size <= max_size, "Image too big ({0}/{1})".format(img_size, max_size)
    check_bounds(RAM_START + 1024, stack_ptr, RAM_START + RAM_SIZE, "Stack pointer invalid", align=(4, 0))
    check_bounds(img_start + 8 * 4, reset_handler, img_start + 90 * 4, "Reset vector invalid")
    check_bounds(reset_handler, nmi_handler, img_start + img_size, "NMI vector invalid")
    check_bounds(reset_handler, hardfault_handler, img_start + img_size, "HardFault vector invalid")
    check_bounds(reset_handler, memmanage_handler, img_start + img_size, "MemManage vector invalid")
    check_bounds(reset_handler, busfault_handler, img_start + img_size, "BusFault vector invalid")
    check_bounds(reset_handler, usagefault_hander, img_start + img_size, "UsageFault vector invalid")
    check_bounds(reset_handler, version_ptr, img_start + FLASH_SECTOR_SIZE, "Version pointer invalid", align=(2, 0))
    ver_offset = version_ptr - img_start
    # check descriptor
    check_bounds(reset_handler, dscr_ptr, img_start + FLASH_SECTOR_SIZE, "Descriptor pointer invalid", align=(4, 0))
    dscr_offset = dscr_ptr - img_start
    size, build_id = IMG_DSCR_FMT.unpack_from(img, dscr_offset)
    assert img_size == size, "Wrong image size in descriptor ({0}/{1})".format(size, img_size)
    
    return img, ver_offset, build_id

def embed_devid(img, ver_offset, devid):
    if devid:
        dev_offset = ver_offset + IMG_VER_FMT.size
        img = bytearray(img)
        IMG_DEV_FMT.pack_into(img, dev_offset, devid)
        img = bytes(img)
    return img

def embed_cfg(img, ver_offset, args):
    cfg_offset = ver_offset + IMG_VER_FMT.size + IMG_DEV_FMT.size
    length, veri_key, srvid, name_len, name = IMG_CFG_FMT.unpack_from(img, cfg_offset)
    assert length >= IMG_CFG_FMT.size, "Wrong configuration length ({0}/{1})".format(length, IMG_CFG_FMT.size)
    if args.key:
        veri_key = vkeyrecode_string(args.key.get_verifying_key().to_string())
    if args.srvid:
        srvid = args.srvid
    if args.name:
        name_len, name = len(args.name), args.name
    img = bytearray(img)
    IMG_CFG_FMT.pack_into(img, cfg_offset, length, veri_key, srvid, name_len, name)
    img = bytes(img)
    return img

def pad(img, align=FLASH_SECTOR_SIZE):
    img += b'\xFF' * (-len(img) % align)
    return img

def sign(text, sign_key):
    if sign_key:
        signature = sign_key.sign(text, sigencode=sigencode_string)
    else:
        signature = HASHFUNC(text).digest()[::-1]
        signature = pad(signature, CURVE.signature_length)
    return text + signature

def make(args):
    fota, ver_offset, fota_id = check_img(args.fota, FOTA_BASE_ADR, FOTA_MAX_SIZE)
    fota = embed_devid(fota, ver_offset, args.devid)
    fota = embed_cfg(fota, ver_offset, args)
    fota = pad(sign(fota, args.key))
    fota_size = len(fota)
    
    app_start = FOTA_BASE_ADR + fota_size
    app_max_size = FLASH_SIZE - BOOT_MAX_SIZE - fota_size
    app, ver_offset, app_id = check_img(args.app, app_start, app_max_size)
    assert app_id == fota_id, "Build ID do not match"
    app = embed_devid(app, ver_offset, args.devid)
    app = sign(app, args.key)
    
    return fota + app


if __name__ == "__main__":
    
    import argparse
    import os.path
    import sys
    
    sys.tracebacklimit = 0
    
    
    def uuid(s):
        return int_to_bytes(int(s.replace('-', ''), base=16), 16, 'little')
    
    def utf8str(s):
        return s.encode('utf8')
    
    parser = argparse.ArgumentParser(description='Build tool for RSL10 FOTA image files.')
    parser.add_argument('--version', action='version', version="%(prog)s " + __version__)
    parser.add_argument('-d', '--devid', dest="devid", metavar='UUID', type=uuid,
                        help="device UUID to embed in the image (default: no ID)")
    parser.add_argument('-s', '--sign', dest="key", metavar='KEY-PEM', type=argparse.FileType('rt'),
                        help="name of signing key PEM file (default: no signing)")
    parser.add_argument('-i', '--srvid', dest="srvid", metavar='UUID', type=uuid,
                        help="advertised UUID to embed in the image (default: DFU service UUID)")
    parser.add_argument('-n', '--name', dest="name", metavar='NAME', type=utf8str,
                        help="advertised name to embed in the image (default: 'ON FOTA RSL10')")
    parser.add_argument('-o', dest="out", metavar='OUT-IMG', type=argparse.FileType('wb'),
                        help="name of output image file (default: <APP-IMG>.fota)")
    parser.add_argument('fota', metavar='FOTA-IMG', type=argparse.FileType('rb'),
                        help="FOTA stack sub-image containing the BLE stack and the DFU component (.bin file)")
    parser.add_argument('app',  metavar='APP-IMG',  type=argparse.FileType('rb'),
                        help="application sub-image (.bin file)")
    args = parser.parse_args()
    
    #print(args)
    if not args.out:
        args.out = open(args.app.name + ".fota", 'wb')
    
    if args.key:
        with args.key:
            sign_key = ecdsa.keys.SigningKey.from_pem(args.key.read(),
                                                      hashfunc=HASHFUNC)
        args.key = sign_key
    
    with args.fota, args.app, args.out:
        args.out.write(make(args))
