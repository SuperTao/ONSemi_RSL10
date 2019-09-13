这个脚本将FOTA文件通过串口发送给RSL10。RSL10此刻在bootloader阶段，与PC上的串口进行通信。

```
运行如下命令，COM2是电脑的COM口，peripheral_server_uart.fota是需要升级的fota文件。
updater.py COM2 peripheral_server_uart.fota
输出结果如下：
Image      : PSBOUND  ver=1.0.0 / FOTA   ver=2.4.1
Bootloader : BOOTLD ver=2.0.2
*****************************************************************************************
```

updater.py分析如下，正常通信的时候，会先发送HELLO命令，然后发送PROG命令，执行完毕之后，发送RESET命令。

```
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
# 打印进度条
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
	# 检查文件的边界是否合法
    def check_bounds(low, adr, high, text, align=(2, 1)):
		# 判断边界合法
        assert adr % align[0] == align[1] and low <= adr < high, text + " (0x{0:08X})".format(adr)
    # align offset to next flash sector boundary (which is 2KiB)
    # 对齐边界， offset = 0
	offset += -offset % (2 * 1024)
	# 从偏移offset开始，解包数据img，按照格式"<9L"
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
	# 读取文件
    img = file.read()
    # evaluate primary application
	# 这一部分应该是fota.bin,包括蓝牙协议栈和升级部分
    img_start, img_size, ver1_info = eval_header(img, 0)
	# 判断文件的开始地址是否合法
    assert (   img_start == BOOT_BASE_ADR
            or img_start ==  APP_BASE_ADR), "Image start address invalid (0x{0:08X})".format(img_start)
    # evaluate secondary application
    if img_size < len(img):
		# 获取app.bin的信息
        _, _, ver2_info = eval_header(img, img_size)
		# 打印Image的信息，例如：Image      : PSBOND  ver=1.0.0 / FOTA   ver=2.4.1
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
# 结构体，4个unsigned Long型数据，"<"表示little-endian
CMD_FMT = struct.Struct("<4L")
# 6个char，一个unsigned short， 6个char,2个 unsigned short
HELLO1_FMT = struct.Struct("<6sH6sHH")
# 6 char， unsigned short, 6 char， 2 unsigned short， 6 char， 1 unsigned short 
HELLO2_FMT = struct.Struct("<6sH6sHH6sH")
# 2 unsigned char
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
	# 将数据打包成结构体，发送出去
    send(com, CMD_FMT.pack(HELLO, 0, 0, 0))

def recv_hello(com):
	# 接收回复的数据
    data = recv(com, HELLO2_FMT.size)
	# 判断回复长度,列表，元组中，项目的个数
    if len(data) == HELLO2_FMT.size:
		# 解包数据，按照struct的格式解析
        return HELLO2_FMT.unpack(data)
    return HELLO1_FMT.unpack(data)

def do_hello(com):
	# 发送hello命令，数据都是0，发送过去
    send_hello(com)
    # 回复hello命令，返回的是一个列表，每个元素是结构体的成员
	param = recv_hello(com)
    ver_info = []
	# 判断回复的数据长度
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
	# 发送prog命令
    send_prog(com, img_start, img_size, hash(img_data))
	# 一边发送数据一边打印"*"，用来显示进度
    for index in range(0, img_size, sect_size):
		# 在数据末尾加上CRC
        msg = append_fcs(img_data[index:index + sect_size])
        # 检查接收数据
		check_resp(NXT_TYPE, *recv_resp(com))
        # 发送数据
		com.write(msg)
		# 打印进度
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
	# 发送restart命令
    send_restart(com)
	# 接命令
    type, code = recv_resp(com)
    if type == END_TYPE and code == UNKNOWN_CMD:
        reset(com, APP)
    else:
        check_resp(END_TYPE, type, code)


def update(com, file, overwrite=False):
    MAX_RETRIES = 2
	# 加载镜像
    img_start, img_size, img, id = load_image(file)
    if img_start < APP_BASE_ADR:
        assert overwrite, "Overwrite of the Bootloader not allowed"
        if not id.startswith(b"BOOT"):
            assert input("Do you really want to overwrite the Bootloader? (yes/no): ").lower() == "yes", "Update aborted" 
    
    reset(com, BOOT)
	# 发送hello命令
    sect_size = do_hello(com)
    retries = 0
    while True:
        try:
			# 记录开始时间
            start = time()
            do_prog(com, img_start, img_size, img, sect_size, show_progress=True)
            # 记录结束时间
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
	# 构造函数
    def __init__(self, port, dll=None, bitrate=1000000, timeout=0.1):
        super(ComPort, self).__init__(port, bitrate, timeout=timeout)
        self.__part_num = 0
        if ComPort.__dll is None:
            try:
				// dll为None
                if not dll:
                    import sys
                    import os.path
                    dll = "CP210xRuntime"
					# 判断操作系统位数
                    if sys.maxsize > 2**32:
                        dll += "-x64"
					# 获取目录
                    progdir = os.path.dirname(sys.argv[0])
					# 获取dll绝对路径
                    dll = os.path.join(progdir, dll)
				# 打开dll
                ComPort.__dll = WinDLL(dll)
            except WindowsError:
                print("Could not find file '{0}'".format(dll), file=sys.stderr)
                #exit(1)
                return
        self.__part_num = self.get_part_number()
    def get_part_number(self):
		# 判断dll是否存在
        if ComPort.__dll is None:
            return 0
		# 创建句柄
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
	# -v或者--version选项，显示版本号
    parser.add_argument('-v', '--version', action='version', version="%(prog)s " + __version__)
    # --force选项,help的内容是帮助信息，使用-h选项的时候会输出，action选项表示--force后面不需要再加对应的参数
	parser.add_argument('--force', action='store_true',
                        help="force overwrite of the bootloader")
	# PORT参数，前面没有附件选项，type指定文件的类型
    parser.add_argument('port', metavar='PORT', type=str,
                        help="COM port of the RSL10 UART")
    parser.add_argument('file', metavar='FILE', type=argparse.FileType('rb'), nargs='?',
                        help="image file (.bin) to download, "
                             "without this parameter currently installed version info is printed")
    args = parser.parse_args()
    # 打开port端口
    with ComPort(args.port) as com:
		# 参看文件file参数是否存在
        if args.file:
			# 打开文件
            with args.file:
				# 端口，文件，force参数
                update(com, args.file, args.force)
        else:
            info(com)
```   