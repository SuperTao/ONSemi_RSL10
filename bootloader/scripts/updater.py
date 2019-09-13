# ----------------------------------------------------------------------------
# Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
# Semiconductor). All Rights Reserved.
#
# This code is the property of ON Semiconductor and may not be redistributed
# in any form without prior written permission from ON Semiconductor.
# The terms of use and warranty for this code are covered by contractual
# agreements between ON Semiconductor and the licensee.
# ----------------------------------------------------------------------------
# upater.py
#!/usr/bin/env python
""" Update Tool for the RSL10.

    Prerequisites:
    - installed Python, version >=2.7 or >=3.4
    - installed module pyserial, version >=3.2
    - CP210xRuntime.dll in the same directory as updater.py
    - SiliconLabs VCP driver version >= 6.7.3
"""
# ----------------------------------------------------------------------------

from __future__ import print_function


__version__ = '2.0.0'

from ctypes import WinDLL, windll, byref
from ctypes.wintypes import HANDLE, WORD, BYTE
from time import time, sleep

import sys
import struct

try:
    import serial
except ImportError:
    print("The module 'pyserial' is not installed! Please install it with 'pip install pyserial'.", file=sys.stderr)
    sys.exit(1)


NO_TRACEBACK = 0
if sys.version_info < (3, ):
    import __builtin__ as builtins
    input = raw_input
    bytes_iter = lambda bstr: (ord(b) for b in bstr)
    def print(*args, **kwargs):
        flush = kwargs.pop('flush', False)
        builtins.print(*args, **kwargs)
        if flush:
            file = kwargs.get('file')
            if file is None: file = sys.stdout
            file.flush()
else:
    bytes_iter = iter
    if sys.version_info < (3, 6, 4):
        NO_TRACEBACK = None



def calc_crc16(data, crc = 0xFFFF):
    for octet in bytes_iter(data):
        octet ^= crc
        octet ^= octet << 4
        octet &= 0xFF
        crc = (crc >> 8) ^ (octet >> 4) ^ (octet << 3) ^ (octet << 8)
    return crc

def append_fcs(data):
    fcs = ~calc_crc16(data) & 0xFFFF
    return data + struct.pack("<H", fcs)

def check_fcs(data):
    assert calc_crc16(data) == 0xF0B8, "bad FCS"
    return data[:-2]



def hash(data):
    CRC32_TABLE = [
        0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
        0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
        0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
        0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000
    ]
    crc = 0
    for octet in bytes_iter(data):
        crc = (crc >> 4) ^ CRC32_TABLE[(crc ^ (octet >> 0)) & 0x0F]   # lower nibble
        crc = (crc >> 4) ^ CRC32_TABLE[(crc ^ (octet >> 4)) & 0x0F]   # upper nibble
    return crc



def decode_version_16bit(code):
    return "{0}.{1}.{2}".format((code >> 12) & 0xF, (code >> 8) & 0xF, (code >> 0) & 0xFF)

def decode_version_32bit(code):
    return ".".join(str(b) for b in struct.pack(">L", code))

ID_UNKNOWN = b"?" * 6
ID_MISSING = b"\x00" * 6

def print_version(type, ver_list):
    ver_str = []
    for id, ver in ver_list:
        if id == ID_MISSING:
            return
        elif id == ID_UNKNOWN:
            ver = ""
        else:
            ver = "ver=" + decode_version_16bit(ver)
        id = id.decode('ascii')
        ver_str.append("{0:6} {1}".format(id, ver))
    print("{0:11}: {1}".format(type, " / ".join(ver_str)))

def print_progress(show, mark="*"):
    if show:
        print(mark, end="", flush=True)


FLASH_START = 0x100000
FLASH_SIZE  = 380 * 1024
RAM_START   = 0x20000000
RAM_SIZE    = 88 * 1024

BOOT_BASE_ADR = FLASH_START
BOOT_MAX_SIZE = 8 * 1024
APP_BASE_ADR  = BOOT_BASE_ADR + BOOT_MAX_SIZE
APP_MAX_SIZE  = FLASH_SIZE - BOOT_MAX_SIZE


def eval_header(img, offset):
    def check_bounds(low, adr, high, text, align=(2, 1)):
        assert adr % align[0] == align[1] and low <= adr < high, text + " (0x{0:08X})".format(adr)
    # align offset to next flash sector boundary (which is 2KiB)
    offset += -offset % (2 * 1024)
    # unpack image header
    (stack_ptr, reset_handler,
    nmi_handler, hardfault_handler,
    memmanage_handler, busfault_handler,
    usagefault_hander, version_ptr, dscr_ptr) = struct.unpack_from("<9L", img, offset)
    # image must start at flash sector boundary (which is 2KiB)
    # and the reset handler is in the 1st sector
    img_start = reset_handler - reset_handler % (2 * 1024)
    img_size = len(img) - offset
    # determin image size
    if  reset_handler < dscr_ptr < img_start + img_size:
        size, = struct.unpack_from("<L", img, offset + dscr_ptr - img_start)
        if 1024 <= size < img_size:
            # add signature size
            img_size = size + 64
    # check image validity
    assert img_start + img_size <= FLASH_START + FLASH_SIZE, "Image too big ({0})".format(img_size)
    check_bounds(RAM_START + 1024, stack_ptr, RAM_START + RAM_SIZE, "Stack pointer invalid", align=(4, 0))
    check_bounds(img_start + 8 * 4, reset_handler, img_start + 90 * 4, "Reset vector invalid")
    check_bounds(reset_handler, nmi_handler, img_start + img_size, "NMI vector invalid")
    check_bounds(reset_handler, hardfault_handler, img_start + img_size, "HardFault vector invalid")
    check_bounds(reset_handler, memmanage_handler, img_start + img_size, "MemManage vector invalid")
    check_bounds(reset_handler, busfault_handler, img_start + img_size, "BusFault vector invalid")
    check_bounds(reset_handler, usagefault_hander, img_start + img_size, "UsageFault vector invalid")
    if version_ptr != 0:
        check_bounds(reset_handler, version_ptr, img_start + img_size, "Version pointer invalid", align=(2, 0))
        app_id, app_ver = struct.unpack_from("<6sH", img, offset + version_ptr - img_start)
    else:
        app_id, app_ver = ID_UNKNOWN, 0
    return img_start, img_size, (app_id, app_ver)

def load_image(file):
    img = file.read()
    # evaluate primary application
    img_start, img_size, ver1_info = eval_header(img, 0)
    assert (   img_start == BOOT_BASE_ADR
            or img_start ==  APP_BASE_ADR), "Image start address invalid (0x{0:08X})".format(img_start)
    # evaluate secondary application
    if img_size < len(img):
        _, _, ver2_info = eval_header(img, img_size)
        print_version("Image", [ver2_info, ver1_info])
    else:
        print_version("Image", [ver1_info])
    # pad image to alignment 8
    img += b'\xFF' * (-len(img) % 8)
    img_size = len(img)
    return img_start, img_size, img, ver1_info[0]




# Reset types
BOOT = 0
APP = 4

# Command types
HELLO = 0
PROG = 1
READ = 2
RESTART = 3

# Response types
NXT_TYPE = 0x55
END_TYPE = 0xAA
# Response codes
NO_ERROR = 0
BAD_MSG = 1
UNKNOWN_CMD = 2
INVALID_CMD = 2
GENERAL_FLASH_FAILURE = 4
WRITE_FLASH_NOT_ENABLED = 5
BAD_FLASH_ADDRESS = 6
ERASE_FLASH_FAILED = 7
BAD_FLASH_LENGTH = 8
INACCESSIBLE_FLASH = 9
FLASH_COPIER_BUSY = 10
PROG_FLASH_FAILED = 11
VERIFY_FLASH_FAILED = 12
NO_VALID_BOOTLODER = 13

CMD_FMT = struct.Struct("<4L")
HELLO1_FMT = struct.Struct("<6sH6sHH")
HELLO2_FMT = struct.Struct("<6sH6sHH6sH")
RESP_FMT = struct.Struct("<2B")


def reset(com, type):
    nRST = 1
    nUPD = 4
    com.write_latch(nRST | nUPD, type)
    com.write_latch(nRST, nRST)
    sleep(0.1)
    com.reset_input_buffer()
    com.write_latch(nUPD, nUPD)
    

def recover(com):
    nUPD = 4
    com.reset_output_buffer()
    sleep(0.1)
    com.write_latch(nUPD, 0)
    sleep(0.1)
    com.write_latch(nUPD, nUPD)
    


def send(com, msg, fcs=True):
    if fcs:
        msg = append_fcs(msg)
    com.reset_input_buffer()
    com.write(msg)
    #com.flush()

def recv(com, max_size, fcs=True):
    if fcs:
        max_size += 2
    data = com.read(max_size)
    assert len(data) > 0, "no data received"
    if fcs:
        data = check_fcs(data)
    return data


def send_hello(com):
    send(com, CMD_FMT.pack(HELLO, 0, 0, 0))

def recv_hello(com):
    data = recv(com, HELLO2_FMT.size)
    if len(data) == HELLO2_FMT.size:
        return HELLO2_FMT.unpack(data)
    return HELLO1_FMT.unpack(data)

def do_hello(com):
    send_hello(com)
    param = recv_hello(com)
    ver_info = []
    if len(param) == 7:
        boot_id, boot_ver, app_id, app_ver, sect_size, app2_id, app2_ver = param
        if app2_id != ID_MISSING:
            ver_info = [(app2_id, app2_ver)]
    else:
        boot_id, boot_ver, app_id, app_ver, sect_size = param
    ver_info.append((app_id, app_ver))
    print_version("Application", ver_info)
    print_version("Bootloader", [(boot_id, boot_ver)])
    return sect_size


def send_prog(com, start, size, hash):
    send(com, CMD_FMT.pack(PROG, start, size, hash))

def recv_resp(com):
    return RESP_FMT.unpack(recv(com, RESP_FMT.size, fcs=False))

def check_resp(expected, type, code):
    assert expected == type and code == NO_ERROR, "Bad Response: {0}/{1}".format(type, code)

def do_prog(com, img_start, img_size, img_data, sect_size, show_progress=False):
    send_prog(com, img_start, img_size, hash(img_data))
    for index in range(0, img_size, sect_size):
        msg = append_fcs(img_data[index:index + sect_size])
        check_resp(NXT_TYPE, *recv_resp(com))
        com.write(msg)
        print_progress(show_progress)
    print_progress(show_progress, "\n")
    check_resp(END_TYPE, *recv_resp(com))

def send_read(com, adr, length):
    send(com, CMD_FMT.pack(READ, adr, length, 0))

def recv_read(com, length):
    data = recv(com, length + 2, fcs=False)
    if len(data) == RESP_FMT.size:
        (type, code), data = RESP_FMT.unpack(data), None
    else:
        type, code, data = END_TYPE, NO_ERROR, check_fcs(data)
    return type, code, data

def do_read(com, adr, length, show_progress=False):
    CHUNK_SIZE = 2048
    chunks = []
    for offset in range(0, length, CHUNK_SIZE):
        size = min(length - offset, CHUNK_SIZE)
        send_read(com, adr + offset, size)
        type, code, data = recv_read(com, size)
        check_resp(END_TYPE, type, code)
        chunks.append(data)
        print_progress(show_progress)
    print_progress(show_progress, "\n")
    return b"".join(chunks)

def send_restart(com):
    send(com, CMD_FMT.pack(RESTART, 0, 0, 0))

def do_restart(com):
    send_restart(com)
    type, code = recv_resp(com)
    if type == END_TYPE and code == UNKNOWN_CMD:
        reset(com, APP)
    else:
        check_resp(END_TYPE, type, code)


def update(com, file, overwrite=False):
    MAX_RETRIES = 2
    img_start, img_size, img, id = load_image(file)
    if img_start < APP_BASE_ADR:
        assert overwrite, "Overwrite of the Bootloader not allowed"
        if not id.startswith(b"BOOT"):
            assert input("Do you really want to overwrite the Bootloader? (yes/no): ").lower() == "yes", "Update aborted" 
    
    reset(com, BOOT)
    sect_size = do_hello(com)
    retries = 0
    while True:
        try:
            start = time()
            do_prog(com, img_start, img_size, img, sect_size, show_progress=True)
            finish = time()
            do_restart(com)
            return img_size / (finish - start)
        except AssertionError:
            print()
            if retries >= MAX_RETRIES:
                break
            retries += 1
            recover(com)
            print("Update failed, trying again...", file=sys.stderr)
    assert False, "Update not possible!"


def info(com):
    reset(com, BOOT)
    do_hello(com)
    do_restart(com)




class ComPort(serial.Serial):
    __dll = None
    ERROR_MSG = "\nWrong COM port driver (Silicon Labs CP210x VCP Driver version >= 6.7.3 needed)"
    def __init__(self, port, dll=None, bitrate=1000000, timeout=0.1):
        super(ComPort, self).__init__(port, bitrate, timeout=timeout)
        self.__part_num = 0
        if ComPort.__dll is None:
            try:
                if not dll:
                    import sys
                    import os.path
                    dll = "CP210xRuntime"
                    if sys.maxsize > 2**32:
                        dll += "-x64"
                    progdir = os.path.dirname(sys.argv[0])
                    dll = os.path.join(progdir, dll)
                ComPort.__dll = WinDLL(dll)
            except WindowsError:
                print("Could not find file '{0}'".format(dll), file=sys.stderr)
                #exit(1)
                return
        self.__part_num = self.get_part_number()
    def get_part_number(self):
        if ComPort.__dll is None:
            return 0
        handle = HANDLE(self._port_handle)
        part_num = BYTE()
        assert ComPort.__dll.CP210xRT_GetPartNumber(handle, byref(part_num)) in [0, 3], "Get Part number failed" + self.ERROR_MSG
        return part_num.value % 10
    def read_latch(self):
        if self.__part_num < 3:
            return 0
        handle = HANDLE(self._port_handle)
        latch = WORD()
        assert ComPort.__dll.CP210xRT_ReadLatch(handle, byref(latch)) == 0, "Read latch failed" + self.ERROR_MSG
        return latch.value
    def write_latch(self, mask, latch):
        if self.__part_num < 3:
            return
        handle = HANDLE(self._port_handle)
        mask = WORD(mask)
        latch = WORD(latch)
        assert ComPort.__dll.CP210xRT_WriteLatch(handle, mask, latch) == 0, "Write latch failed" + self.ERROR_MSG



if __name__ == "__main__":
    
    import argparse
    
    sys.tracebacklimit = NO_TRACEBACK
    
    parser = argparse.ArgumentParser(description='Updates the RSL10 with a firmware image file over UART.')
    parser.add_argument('-v', '--version', action='version', version="%(prog)s " + __version__)
    parser.add_argument('--force', action='store_true',
                        help="force overwrite of the bootloader")
    parser.add_argument('port', metavar='PORT', type=str,
                        help="COM port of the RSL10 UART")
    parser.add_argument('file', metavar='FILE', type=argparse.FileType('rb'), nargs='?',
                        help="image file (.bin) to download, "
                             "without this parameter currently installed version info is printed")
    args = parser.parse_args()
    
    with ComPort(args.port) as com:
        if args.file:
            with args.file:
                update(com, args.file, args.force)
        else:
            info(com)
    
