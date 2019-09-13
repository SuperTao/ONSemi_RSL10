#!/usr/bin/env python3

import serial
from struct import Struct
from collections import namedtuple
from time import time, sleep


# Link Layer Tasks
TASK_ID_LLM          = 0
TASK_ID_LLC          = 1
TASK_ID_LLD          = 2
TASK_ID_DBG          = 3

# BT Controller Tasks
TASK_ID_LM           = 4
TASK_ID_LC           = 5
TASK_ID_LB           = 6
TASK_ID_LD           = 7

TASK_ID_HCI          = 8
TASK_ID_DISPLAY      = 9

# -----------------------------------------------------------------------------------
# --------------------- BLE HL TASK API Identifiers ---------------------------------
# -----------------------------------------------------------------------------------

TASK_ID_L2CC         = 10   # L2CAP Controller Task
TASK_ID_GATTM        = 11   # Generic Attribute Profile Manager Task
TASK_ID_GATTC        = 12   # Generic Attribute Profile Controller Task
TASK_ID_GAPM         = 13   # Generic Access Profile Manager
TASK_ID_GAPC         = 14   # Generic Access Profile Controller

TASK_ID_APP          = 15   # Application API
TASK_ID_AHI          = 16   # Application Host Interface

# -----------------------------------------------------------------------------------
# --------------------- BLE Profile TASK API Identifiers ----------------------------
# -----------------------------------------------------------------------------------
TASK_ID_DISS         = 20   # Device Information Service Server Task
TASK_ID_DISC         = 21   # Device Information Service Client Task

TASK_ID_PROXM        = 22   # Proximity Monitor Task
TASK_ID_PROXR        = 23   # Proximity Reporter Task

TASK_ID_FINDL        = 24   # Find Me Locator Task
TASK_ID_FINDT        = 25   # Find Me Target Task

TASK_ID_HTPC         = 26   # Health Thermometer Collector Task
TASK_ID_HTPT         = 27   # Health Thermometer Sensor Task

TASK_ID_BLPS         = 28   # Blood Pressure Sensor Task
TASK_ID_BLPC         = 29   # Blood Pressure Collector Task

TASK_ID_HRPS         = 30   # Heart Rate Sensor Task
TASK_ID_HRPC         = 31   # Heart Rate Collector Task

TASK_ID_TIPS         = 32   # Time Server Task
TASK_ID_TIPC         = 33   # Time Client Task

TASK_ID_SCPPS        = 34   # Scan Parameter Profile Server Task
TASK_ID_SCPPC        = 35   # Scan Parameter Profile Client Task

TASK_ID_BASS         = 36   # Battery Service Server Task
TASK_ID_BASC         = 37   # Battery Service Client Task

TASK_ID_HOGPD        = 38   # HID Device Task
TASK_ID_HOGPBH       = 39   # HID Boot Host Task
TASK_ID_HOGPRH       = 40   # HID Report Host Task

TASK_ID_GLPS         = 41   # Glucose Profile Sensor Task
TASK_ID_GLPC         = 42   # Glucose Profile Collector Task

TASK_ID_RSCPS        = 43   # Running Speed and Cadence Profile Server Task
TASK_ID_RSCPC        = 44   # Running Speed and Cadence Profile Collector Task

TASK_ID_CSCPS        = 45   # Cycling Speed and Cadence Profile Server Task
TASK_ID_CSCPC        = 46   # Cycling Speed and Cadence Profile Client Task

TASK_ID_ANPS         = 47   # Alert Notification Profile Server Task
TASK_ID_ANPC         = 48   # Alert Notification Profile Client Task

TASK_ID_PASPS        = 49   # Phone Alert Status Profile Server Task
TASK_ID_PASPC        = 50   # Phone Alert Status Profile Client Task

TASK_ID_CPPS         = 51   # Cycling Power Profile Server Task
TASK_ID_CPPC         = 52   # Cycling Power Profile Client Task

TASK_ID_LANS         = 53   # Location and Navigation Profile Server Task
TASK_ID_LANC         = 54   # Location and Navigation Profile Client Task

TASK_ID_IPSS         = 55   # Internet Protocol Support Profile Server Task
TASK_ID_IPSC         = 56   # Internet Protocol Support Profile Client Task

TASK_ID_ENVS         = 57   # Environmental Sensing Profile Server Task
TASK_ID_ENVC         = 58   # Environmental Sensing Profile Client Task

TASK_ID_WSCS         = 59   # Weight Scale Profile Server Task
TASK_ID_WSCC         = 60   # Weight Scale Profile Client Task

TASK_ID_UDSS         = 61   # User Data Service Server Task
TASK_ID_UDSC         = 62   # User Data Service Client Task

TASK_ID_BCSS         = 63   # Body Composition Server Task
TASK_ID_BCSC         = 64   # Body Composition Client Task

TASK_ID_WPTS         = 65   # Wireless Power Transfer Profile Server Task
TASK_ID_WPTC         = 66   # Wireless Power Transfer Profile Client Task

# 240 -> 241 reserved for Audio Mode 0
TASK_ID_AM0          = 240  # BLE Audio Mode 0 Task
TASK_ID_AM0_HAS      = 241  # BLE Audio Mode 0 Hearing Aid Service Task

TASK_ID_INVALID      = 0xFF # Invalid Task Identifier



class VarStruct(Struct):
    def pack(self, *args):
        return super().pack(*args[:-1]) + args[-1]
    def unpack(self, buffer):
        return super().unpack_from(buffer) + (buffer[self.size:], )



class Message(object):
    Header = namedtuple('Header', "id dst src param_len")
    FMT_HDR = Struct('<HBBBBH')
    FMT_PARAM = None
    def __init__(self, id, dst, src, param=None):
        if isinstance(dst, int): dst = (dst, 0)
        if isinstance(src, int): src = (src, 0)
        if self.FMT_PARAM:
            assert isinstance(param, tuple), ("invalid parameter", param)
            param_data = self.FMT_PARAM.pack(*param)
        elif param:
            assert isinstance(param, bytes), ("invalid parameter", param)
            param_data = param
        else:
            param_data = b''
        self.param = param
        self.param_data = param_data
        self.hdr = self.Header(id, dst, src, len(param_data))
    def pack(self):
        assert len(self.param_data) == self.hdr.param_len, "wrong parameter length"
        return self.FMT_HDR.pack(self.hdr.id, self.hdr.dst[0], self.hdr.dst[1],
                                 self.hdr.src[0], self.hdr.src[1], self.hdr.param_len) + self.param_data
    @staticmethod
    def unpack(data):
        assert len(data) >= Message.FMT_HDR.size, ("message too small", data)
        id, dst_type, dst_index, src_type, src_index, param_len = Message.FMT_HDR.unpack_from(data)
        param = data[Message.FMT_HDR.size:]
        assert param_len == len(param), ("wrong parameter length", data)
        cls = Message.__registry.get(id)
        if cls is None:
            msg = Message(id, (dst_type, dst_index), (src_type, src_index), param)
        elif cls.FMT_PARAM:
            assert len(param) >= cls.FMT_PARAM.size, ("message length does not match message size", cls, data)
            param = cls.FMT_PARAM.unpack(param)
            msg = cls((dst_type, dst_index), (src_type, src_index), param)
        elif param_len > 0:
            msg = cls((dst_type, dst_index), (src_type, src_index), param)
        else:
            msg = cls((dst_type, dst_index), (src_type, src_index))
        return msg
    def handle(self, context):
        print(self)
    def __repr__(self):
        cls = type(self)
        if cls is Message:
            return "{}(id=0x{:04X}, dst={!r}, src={!r}, param={!r})".format(
                      cls.__name__, self.hdr.id, self.hdr.dst, self.hdr.src, self.param)
        else:
            return "{}(dst={!r}, src={!r}, param={!r})".format(
                      cls.__name__, self.hdr.dst, self.hdr.src, self.param)
    __registry = {}
    @classmethod
    def register(cls, task_id, msg_num):
        cls.ID = 256 * task_id + msg_num
        assert cls.ID not in cls.__registry, "already registered ID"
        cls.__registry[cls.ID] = cls

def register(task_id, msg_num):
    def decorator(cls):
        cls.register(task_id, msg_num)
        return cls
    return decorator

@register(TASK_ID_APP, 0)
class VersionCmd(Message):
    def __init__(self, dst=TASK_ID_APP, src=TASK_ID_APP):
        super().__init__(self.ID, dst, src)

@register(TASK_ID_APP, 1)
class VersionResp(Message):
    Param = namedtuple('Param', "app_id app_ver rwip_ver_fw rwip_ver_hw sys_ver hw_cid rom_ver")
    FMT_PARAM = Struct('<6sHLLHHL')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_APP, 2)
class PingCmd(Message):
    def __init__(self, dst=TASK_ID_APP, src=TASK_ID_APP, param=b''):
        super().__init__(self.ID, dst, src, param)

@register(TASK_ID_APP, 3)
class PingResp(Message):
    def __init__(self, dst, src, param=b''):
        super().__init__(self.ID, dst, src, param)

@register(TASK_ID_APP, 4)
class LedCmd(Message):
    Param = namedtuple('Param', "on off")
    FMT_PARAM = Struct('<BB')
    def __init__(self, dst=TASK_ID_APP, src=TASK_ID_APP, param=(0, 0)):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_APP, 5)
class LedResp(Message):
    Param = namedtuple('Param', "status")
    FMT_PARAM = Struct('<B')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_APP, 6)
class TxPwrCmd(Message):
    Param = namedtuple('Param', "level atten")
    FMT_PARAM = Struct('<BB')
    def __init__(self, dst=TASK_ID_APP, src=TASK_ID_APP, param=(0, 0)):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_APP, 7)
class TxPwrResp(Message):
    Param = namedtuple('Param', "status")
    FMT_PARAM = Struct('<B')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_APP, 8)
class TestCmd(Message):
    Param = namedtuple('Param', "resp_len seq_num data")
    FMT_PARAM = VarStruct('<HH')
    def __init__(self, dst=TASK_ID_APP, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_APP, 9)
class TestResp(Message):
    Param = namedtuple('Param', "cmd_len seq_num data")
    FMT_PARAM = VarStruct('<HH')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))


# -----------------------------------------------------------------------------------
# --------------------- GAP ---------------------------------------------------------
# -----------------------------------------------------------------------------------

GAP_ROLE_NONE = 0x00
GAP_ROLE_OBSERVER = 0x01
GAP_ROLE_BROADCASTER = 0x02
GAP_ROLE_CENTRAL = 0x05
GAP_ROLE_PERIPHERAL = 0x0A
GAP_ROLE_ALL = 0x0F
GAP_ROLE_DBG_LE_4_0 = 0x80

GAP_RATE_ANY = 0
GAP_RATE_LE_1MBPS = 1
GAP_RATE_LE_2MBPS = 2

GAPM_NO_OP = 0x00
GAPM_RESET = 0x01
GAPM_CANCEL = 0x02
GAPM_SET_DEV_CONFIG = 0x03
GAPM_SET_CHANNEL_MAP = 0x04
GAPM_GET_DEV_VERSION = 0x05
GAPM_GET_DEV_BDADDR = 0x06
GAPM_GET_DEV_ADV_TX_POWER = 0x07
GAPM_GET_WLIST_SIZE = 0x08 
GAPM_ADD_DEV_IN_WLIST = 0x09
GAPM_RMV_DEV_FRM_WLIST = 0x0A
GAPM_CLEAR_WLIST = 0x0B
GAPM_ADV_NON_CONN = 0x0C
GAPM_ADV_UNDIRECT = 0x0D
GAPM_ADV_DIRECT = 0x0E
GAPM_ADV_DIRECT_LDC = 0x0F
GAPM_UPDATE_ADVERTISE_DATA = 0x10
GAPM_SCAN_ACTIVE = 0x11
GAPM_SCAN_PASSIVE = 0x12
GAPM_CONNECTION_DIRECT = 0x13
GAPM_CONNECTION_AUTO = 0x14
GAPM_CONNECTION_SELECTIVE = 0x15
GAPM_CONNECTION_NAME_REQUEST = 0x16
GAPM_RESOLV_ADDR = 0x17
GAPM_GEN_RAND_ADDR = 0x18
GAPM_USE_ENC_BLOCK = 0x19
GAPM_GEN_RAND_NB = 0x1A 
GAPM_PROFILE_TASK_ADD = 0x1B
GAPM_DBG_GET_MEM_INFO = 0x1C
GAPM_PLF_RESET = 0x1D
GAPM_SET_SUGGESTED_DFLT_LE_DATA_LEN = 0x1E
GAPM_GET_SUGGESTED_DFLT_LE_DATA_LEN = 0x1F
GAPM_GET_MAX_LE_DATA_LEN = 0x20
GAPM_GET_RAL_SIZE = 0x21
GAPM_GET_RAL_LOC_ADDR = 0x22
GAPM_GET_RAL_PEER_ADDR = 0x23
GAPM_ADD_DEV_IN_RAL = 0x24
GAPM_RMV_DEV_FRM_RAL = 0x25
GAPM_CLEAR_RAL = 0x26
GAPM_CONNECTION_GENERAL = 0x27
GAPM_SET_IRK = 0x28
GAPM_LEPSM_REG = 0x29
GAPM_LEPSM_UNREG = 0x2A
GAPM_LE_TEST_STOP = 0x2B
GAPM_LE_TEST_RX_START = 0x2C 
GAPM_LE_TEST_TX_START = 0x2D
GAPM_GEN_DH_KEY = 0x2E
GAPM_GET_PUB_KEY = 0x2F
GAPM_ENABLE_POWER_SAVE = 0x30

GAPM_CFG_ADDR_PUBLIC = 0
GAPM_CFG_ADDR_PRIVATE = 1
GAPM_CFG_ADDR_HOST_PRIVACY = 2
GAPM_CFG_ADDR_CTNL_PRIVACY = 4

GAPC_NO_OP = 0x00
GAPC_DISCONNECT = 0x01
GAPC_GET_PEER_NAME = 0x02
GAPC_GET_PEER_VERSION = 0x03
GAPC_GET_PEER_FEATURES = 0x04
GAPC_GET_PEER_APPEARANCE = 0x05
GAPC_GET_PEER_SLV_PREF_PARAMS = 0x06
GAPC_GET_CON_RSSI = 0x07
GAPC_GET_CON_CHANNEL_MAP = 0x08
GAPC_UPDATE_PARAMS = 0x09
GAPC_BOND = 0x0A
GAPC_ENCRYPT = 0x0B
GAPC_SECURITY_REQ = 0x0C
GAPC_OP_DEPRECATED_0 = 0x0D
GAPC_OP_DEPRECATED_1 = 0x0E
GAPC_OP_DEPRECATED_2 = 0x0F
GAPC_OP_DEPRECATED_3 = 0x10
GAPC_OP_DEPRECATED_4 = 0x11
GAPC_GET_LE_PING_TO = 0x12
GAPC_SET_LE_PING_TO = 0x13
GAPC_SET_LE_PKT_SIZE = 0x14
GAPC_GET_ADDR_RESOL_SUPP = 0x15
GAPC_KEY_PRESS_NOTIFICATION = 0x16
GAPC_SET_PHY = 0x17
GAPC_GET_PHY = 0x18
GAPC_SET_PREF_SLAVE_LATENCY = 0x19
GAPC_SIGN_PACKET = 0x1A
GAPC_SIGN_CHECK = 0x1B

GATTC_NO_OP = 0x00
GATTC_MTU_EXCH = 0x01
GATTC_DISC_ALL_SVC = 0x02
GATTC_DISC_BY_UUID_SVC = 0x03
GATTC_DISC_INCLUDED_SVC = 0x04
GATTC_DISC_ALL_CHAR = 0x05
GATTC_DISC_BY_UUID_CHAR = 0x06
GATTC_DISC_DESC_CHAR = 0x07
GATTC_READ = 0x08
GATTC_READ_LONG = 0x09
GATTC_READ_BY_UUID = 0x0A
GATTC_READ_MULTIPLE = 0x0B
GATTC_WRITE = 0x0C
GATTC_WRITE_NO_RESPONSE = 0x0D
GATTC_WRITE_SIGNED = 0x0E
GATTC_EXEC_WRITE = 0x0F
GATTC_REGISTER = 0x10
GATTC_UNREGISTER = 0x11
GATTC_NOTIFY = 0x12
GATTC_INDICATE = 0x13
GATTC_SVC_CHANGED = 0x14
GATTC_SDP_DISC_SVC = 0x15
GATTC_SDP_DISC_SVC_ALL = 0x16
GATTC_SDP_DISC_CANCEL = 0x17

@register(TASK_ID_GAPM, 0)
class GAPM_CMP_EVT(Message):
    Param = namedtuple('Param', "operation status")
    FMT_PARAM = Struct('<BB')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPM, 2)
class GAPM_RESET_CMD(Message):
    Param = namedtuple('Param', "operation")
    FMT_PARAM = Struct('<B')
    def __init__(self, dst=TASK_ID_GAPM, src=TASK_ID_APP, param=(GAPM_RESET, )):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPM, 4)
class GAPM_SET_DEV_CONFIG_CMD(Message):
    Param = namedtuple('Param', "operation role renew_dur addr irk addr_type "
                                 "pairing_mode gap_start_hdl gatt_start_hdl "
                                 "att_cfg sugg_max_tx_octets sugg_max_tx_time "
                                 "max_mtu max_mps max_nb_lecb audio_cfg "
                                 "tx_pref_rates rx_pref_rates")
    FMT_PARAM = Struct('<BBH6s16sBBHHHHHHHBxHBB')
    def __init__(self, dst=TASK_ID_GAPM, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPM, 6)
class GAPM_GET_DEV_INFO_CMD(Message):
    Param = namedtuple('Param', "operation")
    FMT_PARAM = Struct('<B')
    def __init__(self, dst=TASK_ID_GAPM, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPM, 7)
class GAPM_DEV_VERSION_IND(Message):
    Param = namedtuple('Param', "hci_ver lmp_ver host_ver hci_subver "
                                "lmp_subver host_subver manuf_name")
    FMT_PARAM = Struct('<BBBxHHHH')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        version = 0x1000000 * self.param.hci_ver  + 0x100 * self.param.hci_subver
        print("HCI :", decode_version_32bit(version))
        version = 0x1000000 * self.param.lmp_ver  + 0x100 * self.param.lmp_subver
        print("LMP :", decode_version_32bit(version))
        version = 0x1000000 * self.param.host_ver + 0x100 * self.param.host_subver
        print("HOST:", decode_version_32bit(version))
        print("MANU: {:04X}".format(self.param.manuf_name))

@register(TASK_ID_GAPM, 8)
class GAPM_DEV_BDADDR_IND(Message):
    Param = namedtuple('Param', "addr addr_type")
    FMT_PARAM = Struct('<6sB')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("Device Address:", decode_addr(self.param.addr),
              "public" if self.param.addr_type == GAPM_CFG_ADDR_PUBLIC else "random")

@register(TASK_ID_GAPM, 9)
class GAPM_DEV_ADV_TX_POWER_IND(Message):
    Param = namedtuple('Param', "power_lvl")
    FMT_PARAM = Struct('<b')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("TX power: {}dB".format(self.param.power_lvl))

@register(TASK_ID_GAPM, 30)
class GAPM_GET_SUGG_DFLT_DATA_LEN_IND(Message):
    Param = namedtuple('Param', "suggted_max_tx_octets suggted_max_tx_time")
    FMT_PARAM = Struct('<HH')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("Suggested max TX data length:", self.param.suggted_max_tx_octets)
        print("Suggested max TX data time  :", self.param.suggted_max_tx_time)

@register(TASK_ID_GAPM, 31)
class GAPM_MAX_DATA_LEN_IND(Message):
    Param = namedtuple('Param', "suppted_max_tx_octets suggted_max_tx_time "
                                 "suppted_max_rx_octets suppted_max_rx_time")
    FMT_PARAM = Struct('<HHHH')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("Supported max TX data length:", self.param.suppted_max_tx_octets)
        print("Supported max TX data time  :", self.param.suggted_max_tx_time)
        print("Supported max RX data length:", self.param.suppted_max_rx_octets)
        print("Supported max RX data time  :", self.param.suppted_max_rx_time)

@register(TASK_ID_GAPM, 17)
class GAPM_START_CONNECTION_CMD(Message):
    Param = namedtuple('Param', "op_code op_addr_src op_state scan_interval scan_window "
                                "con_intv_min con_intv_max con_latency superv_to ce_len_min ce_len_max "
                                "nb_peers peer_addr peer_addr_type")
    FMT_PARAM = Struct('<BBHHHHHHHHHB6sB')
    def __init__(self, dst=TASK_ID_GAPM, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 0)
class GAPC_CMP_EVT(Message):
    Param = namedtuple('Param', "operation status")
    FMT_PARAM = Struct('<BB')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 1)
class GAPC_CONNECTION_REQ_IND(Message):
    Param = namedtuple('Param', "conhdl con_interval con_latency sup_to clk_accuracy "
                                 "peer_addr_type peer_addr")
    FMT_PARAM = Struct('<HHHHBB6s')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        context.conid = self.hdr.src
        context.send_msg(GAPC_CONNECTION_CFM(dst=context.conid))
        print("Connected handle={}".format(self.param.conhdl))
        print("Peer Address       :", decode_addr(self.param.peer_addr),
              "public" if self.param.peer_addr_type == GAPM_CFG_ADDR_PUBLIC else "random")
        print("Connection interval:", self.param.con_interval)
        print("Connection latency :", self.param.con_latency)
        print("Supervision timeout:", self.param.sup_to)
        print("Clock accuracy     :", self.param.clk_accuracy)

@register(TASK_ID_GAPC, 2)
class GAPC_CONNECTION_CFM(Message):
    Param = namedtuple('Param', "lcsrk lsign_counter rcsrk rsign_counter auth "
                                "svc_changed_ind_enable ltk_present")
    FMT_PARAM = Struct('<16sL16sLBBB')
    def __init__(self, dst, src=TASK_ID_APP, param=(b'', 0, b'', 0, 0, 0, 0)):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 3)
class GAPC_DISCONNECT_IND(Message):
    Param = namedtuple('Param', "conhdl reason")
    FMT_PARAM = Struct('<HBx')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        if context.conid == self.hdr.src:
            context.conid = None
        print("Disconnected handle={}, reason={}".format(self.param.conhdl, hex(self.param.reason)))

@register(TASK_ID_GAPC, 4)
class GAPC_DISCONNECT_CMD(Message):
    Param = namedtuple('Param', "operation reason")
    FMT_PARAM = Struct('<BB')
    def __init__(self, dst, src=TASK_ID_APP, param=(GAPC_DISCONNECT, 0x13)):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 5)
class GAPC_GET_INFO_CMD(Message):
    Param = namedtuple('Param', "operation")
    FMT_PARAM = Struct('<B')
    def __init__(self, dst=TASK_ID_GAPM, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 7)
class GAPC_PEER_VERSION_IND(Message):
    Param = namedtuple('Param', "compid lmp_subver lmp_ver")
    FMT_PARAM = Struct('<HHBx')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        version = 0x1000000 * self.param.lmp_ver  + 0x100 * self.param.lmp_subver
        print("Peer Version")
        print("LMP :", decode_version_32bit(version))
        print("MANU: {:04X}".format(self.param.compid))

@register(TASK_ID_GAPC, 15)
class GAPC_PARAM_UPDATE_REQ_IND(Message):
    Param = namedtuple('Param', "intv_min intv_max latency time_out")
    FMT_PARAM = Struct('<HHHH')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        if context.conid == self.hdr.src:
            context.send_msg(GAPC_PARAM_UPDATE_CFM(dst=context.conid))
        print("Param Update Request")
        print("Connection min interval:", self.param.intv_min)
        print("Connection max interval:", self.param.intv_max)
        print("Connection latency     :", self.param.latency)
        print("Supervision timeout    :", self.param.time_out)

@register(TASK_ID_GAPC, 16)
class GAPC_PARAM_UPDATE_CFM(Message):
    Param = namedtuple('Param', "accept ce_len_min ce_len_max")
    FMT_PARAM = Struct('<BxHH')
    def __init__(self, dst, src=TASK_ID_APP, param=(True, 0, 0)):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 17)
class GAPC_PARAM_UPDATED_IND(Message):
    Param = namedtuple('Param', "con_interval con_latency sup_to")
    FMT_PARAM = Struct('<HHH')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("Param Update")
        print("Connection interval:", self.param.con_interval)
        print("Connection latency :", self.param.con_latency)
        print("Supervision timeout:", self.param.sup_to)

@register(TASK_ID_GAPC, 43)
class GAPC_SET_LE_PKT_SIZE_CMD(Message):
    Param = namedtuple('Param', "operation tx_octets tx_time")
    FMT_PARAM = Struct('<BxHH')
    def __init__(self, dst, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 44)
class GAPC_LE_PKT_SIZE_IND(Message):
    Param = namedtuple('Param', "max_tx_octets max_tx_time max_rx_octets max_rx_time")
    FMT_PARAM = Struct('<HHHH')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("LE Param Update")
        print("Max TX packet size:", self.param.max_tx_octets)
        print("Max TX packet time:", self.param.max_tx_time)
        print("Max RX packet size:", self.param.max_rx_octets)
        print("Max RX packet time:", self.param.max_rx_time)

@register(TASK_ID_GAPC, 47)
class GAPC_SET_PHY_CMD(Message):
    Param = namedtuple('Param', "operation tx_rate rx_rate")
    FMT_PARAM = Struct('<BBB')
    def __init__(self, dst, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GAPC, 48)
class GAPC_LE_PHY_IND(Message):
    Param = namedtuple('Param', "tx_rate rx_rate")
    FMT_PARAM = Struct('<BB')
    def __init__(self, dst, src, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("PHY Param Update")
        print("TX rate:", self.param.tx_rate)
        print("RX rate:", self.param.rx_rate)

@register(TASK_ID_GATTC, 0)
class GATTC_CMP_EVT(Message):
    Param = namedtuple('Param', "operation status seq_num")
    FMT_PARAM = Struct('<BBH')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GATTC, 1)
class GATTC_EXC_MTU_CMD(Message):
    Param = namedtuple('Param', "operation seq_num")
    FMT_PARAM = Struct('<BxH')
    def __init__(self, dst, src=TASK_ID_APP, param=(GATTC_MTU_EXCH, 0)):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GATTC, 2)
class GATTC_MTU_CHANGED_IND(Message):
    Param = namedtuple('Param', "mtu seq_num")
    FMT_PARAM = Struct('<HH')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("ATT MTU Changed")
        print("MTU:", self.param.mtu)

@register(TASK_ID_GATTC, 8)
class GATTC_READ_SIMPLE_CMD(Message):
    Param = namedtuple('Param', "operation nb seq_num handle offset length")
    FMT_PARAM = Struct('<BBHHHH')
    def __init__(self, dst, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))

@register(TASK_ID_GATTC, 9)
class GATTC_READ_IND(Message):
    Param = namedtuple('Param', "handle offset length value")
    FMT_PARAM = VarStruct('<HHH')
    def __init__(self, dst, src, param):
        super().__init__(self.ID, dst, src, self.Param(*param))
    def handle(self, context):
        print("Characteristic handle={}".format(self.param.handle))
        print("Offset:", self.param.offset)
        print("Length:", self.param.length)
        print("Value :", repr(self.param.value))

@register(TASK_ID_GATTC, 10)
class GATTC_WRITE_CMD(Message):
    Param = namedtuple('Param', "operation auto_execute seq_num handle offset length cursor value")
    FMT_PARAM = VarStruct('<BBHHHHH')
    def __init__(self, dst, src=TASK_ID_APP, param=()):
        super().__init__(self.ID, dst, src, self.Param(*param))




def calc_crc(data):
    crc = 0xFFFF
    for octet in data:
        octet ^= crc
        octet ^= octet << 4
        octet &= 0xFF
        crc = (crc >> 8) ^ (octet >> 4) ^ (octet << 3) ^ (octet << 8)
    return crc

def append_fcs(data):
    fcs = ~calc_crc(data) & 0xFFFF
    return data + fcs.to_bytes(2, 'little')

def check_fcs(data):
    assert calc_crc(data) == 0xF0B8, "bad FCS"
    return data[:-2]


MAX_BLOCK_LEN = 254

def cobs_encode(inp):
    #return b''.join((len(x) + 1).to_bytes(1, 'little') + x for x in inp.split(b'\x00'))
    out = []
    for seg in inp.split(b'\x00'):
        seg_len = len(seg)
        if seg_len <= MAX_BLOCK_LEN:
            out.append((seg_len + 1).to_bytes(1, 'little'))
            out.append(seg)
        else:
            for index in range(0, seg_len, MAX_BLOCK_LEN):
                block = seg[index:index + MAX_BLOCK_LEN]
                out.append((len(block) + 1).to_bytes(1, 'little'))
                out.append(block)
    return b''.join(out)

def cobs_decode(inp):
    inp_len = len(inp)
    index = 0
    block = b''
    out = []
    while index < inp_len:
        code = inp[index]
        block += inp[index + 1:index + code]
        if code <= MAX_BLOCK_LEN:
            out.append(block)
            block = b''
        index += code
    if block:
        out.append(block)
    return b'\x00'.join(out)



def decode_version_16bit(code):
    return "{}.{}.{}".format((code >> 12) & 0xF, (code >> 8) & 0xF, (code >> 0) & 0xFF)

def decode_version_32bit(code):
    return ".".join(str(b) for b in code.to_bytes(4, 'big'))

def decode_addr(addr):
    return ":".join(format(a, "02X") for a in reversed(addr))

def encode_addr(addr):
    return int(addr.replace(':', ''), base=16).to_bytes(6, 'little')


class Dongle():
    FLAG = b'\x00'
    def __init__(self, port, timeout=0.5):
        self.conid = None
        self._pending_frame = b''
        self._port = serial.Serial("COM{}".format(port), 1000000, timeout=timeout)
    def close(self):
        self._port.close()
    
    def send_frame(self, data, back2back=False):
        FLAG = Dongle.FLAG
        frame  = FLAG if not back2back else b''
        frame += data + FLAG
        self._port.write(frame)
        return frame
    def recv_frame(self):
        FLAG = Dongle.FLAG
        port, frame = self._port, self._pending_frame
        while True:
            frame += port.read_all()
            try:
                pos = frame.index(FLAG)
            except ValueError:
                pos = -1
            if pos > 0:
                break
            if pos == 0:
                frame = frame[1:]
            else:
                data = port.read(1)
                assert data, "receive timeout"
                frame += data
        self._pending_frame = frame[pos+1:]
        return frame[:pos]
    
    def send_msg(self, msg):
        data = cobs_encode(append_fcs(msg.pack()))
        self.send_frame(data)
    def recv_msg(self, handler=lambda m, c: False):
        while True:
            data = cobs_decode(self.recv_frame())
            try:
                msg = Message.unpack(check_fcs(data))
            except AssertionError as e:
                print(*e.args)
            else:
                if handler(msg, self):
                    return msg
                msg.handle(self)
    
    def version(self):
        self.send_msg(VersionCmd())
        param = self.recv_msg(lambda m, c: isinstance(m, VersionResp)).param
        return "{}: ver={} sys={} rwip_fw={} rwip_hw={} rom=0x{:08X} cid=0x{:04X}".format(
                      param.app_id.decode(),
                      decode_version_16bit(param.app_ver),
                      decode_version_16bit(param.sys_ver),
                      decode_version_32bit(param.rwip_ver_fw),
                      decode_version_32bit(param.rwip_ver_hw),
                      param.rom_ver, param.hw_cid)

    def ping(self, hello="PONG"):
        self.send_msg(PingCmd(param=hello.encode('utf8')))
        param = self.recv_msg(lambda m, c: isinstance(m, PingResp)).param
        return param.decode('utf8')
    
    def led(self, on=0, off=0):
        self.send_msg(LedCmd(param=(on, off)))
        param = self.recv_msg(lambda m, c: isinstance(m, LedResp)).param
        return ", ".join("LED{}={:d}".format(bit, (1 << bit) & param.status != 0) for bit in range(2))

    def txpwr(self, level=0, atten=0):
        self.send_msg(TxPwrCmd(param=(level, atten)))
        param = self.recv_msg(lambda m, c: isinstance(m, TxPwrResp)).param
        return "OK" if param.status != 0 else "FAILED"
    
    def test(self, txlen, rxlen, wnd=1, dur=1):
        data = b'X' * txlen
        seq_errors, tim_errors = 0, 0
        txcnt, rxcnt = 0, 0
        msg = TestCmd(param=(rxlen, txcnt, data))
        frame = cobs_encode(append_fcs(msg.pack()))
        back2back = False
        now = time()
        end = now + dur
        while now < end:
            self.send_frame(frame, back2back)
            txcnt += 1
            msg = TestCmd(param=(rxlen, txcnt & 0xFFFF, data))
            frame = cobs_encode(append_fcs(msg.pack()))
            back2back = True
            while txcnt - rxcnt >= wnd:
                try:
                    param = self.recv_msg(lambda m, c: isinstance(m, TestResp)).param
                except AssertionError:
                    tim_errors += 1
                    back2back = False
                else:
                    if param.seq_num != (rxcnt & 0xFFFF):
                        #print("expected:", rxcnt, "got:", param.seq_num)
                        seq_errors += 1
                rxcnt += 1
            now = time()
        while rxcnt < txcnt:
            try:
                param = self.recv_msg(lambda m, c: isinstance(m, TestResp)).param
            except AssertionError:
                tim_errors += 1
                break
            if param.seq_num != (rxcnt & 0xFFFF):
                #print("expected:", rxcnt, "got:", param.seq_num)
                seq_errors += 1
            rxcnt += 1
        return txcnt, seq_errors, tim_errors

    def gapm_reset(self):
        self.send_msg(GAPM_RESET_CMD())
        param = self.recv_msg(lambda m, c: isinstance(m, GAPM_CMP_EVT)
                                            and m.param.operation == GAPM_RESET).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
    
    def gapm_set_dev_config(self, role, renew_dur=15000, addr=b'', irk=b'', addr_type=0,
                            pairing_mode=0, gap_start_hdl=0, gatt_start_hdl=0,
                            att_cfg=0x0080, sugg_max_tx_octets=27, sugg_max_tx_time=0,
                            max_mtu=0x200, max_mps=0x200, max_nb_lecb=0, audio_cfg=0,
                            tx_pref_rates=GAP_RATE_ANY, rx_pref_rates=GAP_RATE_ANY):
        if sugg_max_tx_time == 0: sugg_max_tx_time = (14 * 8 + sugg_max_tx_octets * 8)
        param = (GAPM_SET_DEV_CONFIG, role, renew_dur, addr, irk, addr_type,
                 pairing_mode, gap_start_hdl, gatt_start_hdl, att_cfg,
                 sugg_max_tx_octets, sugg_max_tx_time, max_mtu, max_mps,
                 max_nb_lecb, audio_cfg, tx_pref_rates, rx_pref_rates)
        self.send_msg(GAPM_SET_DEV_CONFIG_CMD(param=param))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPM_CMP_EVT)
                                            and m.param.operation == GAPM_SET_DEV_CONFIG).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))

    def gapm_get_dev_info(self, operation):
        self.send_msg(GAPM_GET_DEV_INFO_CMD(param=(operation, )))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPM_CMP_EVT)
                                            and m.param.operation == operation).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))

    def gapm_start_connection(self, op_code, op_addr_src, scan_interval=48, scan_window=48,
                              con_intv_min=6, con_intv_max=6, con_latency=0, superv_to=10,
                              ce_len_min=0, ce_len_max=0, peer_addr=b'', peer_addr_type=0):
        param = (op_code, op_addr_src, 0, scan_interval, scan_window,
                 con_intv_min, con_intv_max, con_latency, superv_to,
                 ce_len_min, ce_len_max, 1, peer_addr, peer_addr_type)
        self.send_msg(GAPM_START_CONNECTION_CMD(param=param))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPM_CMP_EVT)
                                            and m.param.operation == op_code).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gapc_disconnect(self, reason=0x13):
        if self.conid is None:
            return
        tskid = self.conid
        self.send_msg(GAPC_DISCONNECT_CMD(dst=tskid, param=(GAPC_DISCONNECT, reason)))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == GAPC_DISCONNECT).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gapc_get_info(self, operation):
        if self.conid is None:
            return
        tskid = self.conid
        self.send_msg(GAPC_GET_INFO_CMD(dst=tskid, param=(operation, )))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == operation).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gapc_set_le_pkt_size(self, size=27, time=0):
        if self.conid is None:
            return
        if time == 0: time = (14 * 8 + size * 8)
        tskid = self.conid
        self.send_msg(GAPC_SET_LE_PKT_SIZE_CMD(dst=tskid, param=(GAPC_SET_LE_PKT_SIZE, size, time)))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == GAPC_SET_LE_PKT_SIZE).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gapc_set_phy(self, tx=GAP_RATE_ANY, rx=GAP_RATE_ANY):
        if self.conid is None:
            return
        tskid = self.conid
        self.send_msg(GAPC_SET_PHY_CMD(dst=tskid, param=(GAPC_SET_PHY, tx, rx)))
        param = self.recv_msg(lambda m, c: isinstance(m, GAPC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == GAPC_SET_PHY).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gattc_exc_mtu(self, seq_num=0):
        if self.conid is None:
            return
        tskid = (TASK_ID_GATTC, self.conid[1])
        param = (GATTC_MTU_EXCH, seq_num)
        self.send_msg(GATTC_EXC_MTU_CMD(dst=tskid, param=param))
        param = self.recv_msg(lambda m, c: isinstance(m, GATTC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == GATTC_MTU_EXCH).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gattc_read(self, operation=GATTC_READ, nb=0, seq_num=0, handle=0, offset=0, length=0):
        if self.conid is None:
            return
        tskid = (TASK_ID_GATTC, self.conid[1])
        param = (operation, nb, seq_num, handle, offset, length)
        self.send_msg(GATTC_READ_SIMPLE_CMD(dst=tskid, param=param))
        param = self.recv_msg(lambda m, c: isinstance(m, GATTC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == operation).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
 
    def gattc_write(self, operation=GATTC_WRITE, auto_execute=True, seq_num=0, handle=0, offset=0, value=b''):
        if self.conid is None:
            return
        tskid = (TASK_ID_GATTC, self.conid[1])
        param = (operation, auto_execute, seq_num, handle, offset, len(value), 0, value)
        self.send_msg(GATTC_WRITE_CMD(dst=tskid, param=param))
        param = self.recv_msg(lambda m, c: isinstance(m, GATTC_CMP_EVT)
                                            and m.hdr.src == tskid
                                            and m.param.operation == operation).param
        return "OK" if param.status == 0 else "FAILED ({})".format(hex(param.status))
   


def enable_transfer(dong):
    if dong.conid is None:
        return 0
    dong.gattc_write(GATTC_WRITE, handle=13, value=(1).to_bytes(2, 'little'));
    try:
        dong.recv_msg()
    except AssertionError:
        pass

def transfer(dong, h=12, w=1, t=1, l=20, v=None):
    if dong.conid is None:
        return 0
    
    if v is None:
        v = b"1234567890" * 50
    DATA_FMT = Struct("<LH{}s".format(l - 6))
    
    seq_errors, tim_errors = 0, 0
    txcnt, rxcnt = 0, 0
    tskid = (TASK_ID_GATTC, dong.conid[1])
    def check_write_cmp(m, c):
        return (isinstance(m, GATTC_CMP_EVT) and
                m.hdr.src == tskid and
                m.param.operation == GATTC_WRITE_NO_RESPONSE)
    
    data = DATA_FMT.pack(txcnt, l, v)
    param = (GATTC_WRITE_NO_RESPONSE, True, txcnt & 0xFFFF, h, 0, len(data), 0, data)
    msg = GATTC_WRITE_CMD(dst=tskid, param=param)
    now = time()
    end = now + t
    while now < end:
        dong.send_msg(msg)
        txcnt += 1
        data = DATA_FMT.pack(txcnt, l, v)
        param = (GATTC_WRITE_NO_RESPONSE, True, txcnt & 0xFFFF, h, 0, len(data), 0, data)
        msg = GATTC_WRITE_CMD(dst=tskid, param=param)
        while txcnt - rxcnt >= w:
            try:
                param = dong.recv_msg(check_write_cmp).param
            except AssertionError:
                tim_errors += 1
                txcnt -= 1
                data = DATA_FMT.pack(txcnt, l, v)
                param = (GATTC_WRITE_NO_RESPONSE, True, txcnt & 0xFFFF, h, 0, len(data), 0, data)
                msg = GATTC_WRITE_CMD(dst=tskid, param=param)
                break
            if param.seq_num != (rxcnt & 0xFFFF):
                #print("expected:", rxcnt, "got:", param.seq_num)
                seq_errors += 1
            if param.status > 0:
                print("FAILED ({})".format(hex(param.status)))
                return rxcnt, seq_errors, tim_errors
            rxcnt += 1
        now = time()
    while txcnt != rxcnt:
        try:
            status = dong.recv_msg(check_write_cmp).param.status
        except AssertionError:
            tim_errors += 1
            break
        if status > 0:
            print("FAILED ({})".format(hex(status)))
            return rxcnt, seq_errors, tim_errors
        rxcnt += 1
    
    print("throughput:", txcnt * l / t)
    return txcnt, seq_errors, tim_errors


"""

dong=test.Dongle(27)
dong.gapm_set_dev_config(test.GAP_ROLE_CENTRAL, sugg_max_tx_octets=251)
dong.gapm_start_connection(test.GAPM_CONNECTION_DIRECT, 0, peer_addr=test.encode_addr("33:7e:18:d4:ad:e1"), peer_addr_type=test.GAPM_CFG_ADDR_PRIVATE, superv_to=200, con_intv_min=6, con_intv_max=12, ce_len_min=0, ce_len_max=0xffff)
dong.gapc_set_le_pkt_size(251)
dong.gattc_exc_mtu()
dong.gapc_set_phy(2,0)
test.transfer(dong, l=492, t=10, w=2)
dong.gapc_disconnect()
dong.close()


def ping(dong, w=1, t=1, l=20):
    hello = 'A' * l
    msg = test.PingCmd(param=hello.encode('utf8'))
    data = test.cobs_encode(test.append_fcs(msg.pack()))
    tim_errors = 0
    txcnt, rxcnt = 0, 0
    back2back = False
    now = time.time()
    end = now + t
    while now < end:
        dong.send_frame(data, back2back)
        txcnt += 1
        back2back = True
        while txcnt - rxcnt >= w:
            try:
                dong.recv_msg(lambda m, c: isinstance(m, test.PingResp))
            except AssertionError:
                tim_errors += 1
                back2back = False
            rxcnt += 1
        now = time.time()
    while rxcnt < txcnt:
        try:
            dong.recv_msg(lambda m, c: isinstance(m, test.PingResp))
        except AssertionError:
            tim_errors += 1
            break
        rxcnt += 1
    return txcnt, rxcnt, tim_errors

def tt(self, txlen, rxlen, wnd=1, dur=1):
    data = b'X' * txlen
    seq_errors, tim_errors = 0, 0
    txcnt, rxcnt = 0, 0
    cmd = test.TestCmd(param=(rxlen, txcnt, data))
    frame = test.cobs_encode(test.append_fcs(cmd.pack()))
    back2back = False
    now = time()
    end = now + dur
    start = now
    while now < end:
        self.send_frame(frame, back2back)
        print("{} {:4} {:4} {:>10.3f}".format("send", txcnt, "", (time() - start) * 1000))
        txcnt += 1
        cmd = test.TestCmd(param=(rxlen, txcnt & 0xFFFF, data))
        frame = test.cobs_encode(test.append_fcs(cmd.pack()))
        back2back = True
        while txcnt - rxcnt >= wnd:
            try:
                resp = self.recv_msg(lambda m, c: isinstance(m, test.TestResp))
                param = resp.param
                print("{} {:4} {:4} {:>10.3f}".format("recv", rxcnt, param.seq_num, (time() - start) * 1000))
            except AssertionError:
                tim_errors += 1
                back2back = False
                print("timo", rxcnt)
            else:
                if param.seq_num != (rxcnt & 0xFFFF):
                    #print("expected:", rxcnt, "got:", param.seq_num)
                    seq_errors += 1
            rxcnt += 1
        now = time()
    self._port.flushOutput()
    end = time()
    while rxcnt < txcnt:
        try:
            param = self.recv_msg(lambda m, c: isinstance(m, test.TestResp)).param
            print("{} {:4} {:4} {:>10.3f}".format("recv", rxcnt, param.seq_num, (time() - start) * 1000))
        except AssertionError:
            tim_errors += 1
            print("timo", rxcnt)
            break
        if param.seq_num != (rxcnt & 0xFFFF):
            #print("expected:", rxcnt, "got:", param.seq_num)
            seq_errors += 1
        rxcnt += 1
    tx_len = len(test.cobs_encode(test.append_fcs(cmd.pack()))) * txcnt
    rx_len = len(test.cobs_encode(test.append_fcs(resp.pack()))) * rxcnt
    print("TX throughput:", tx_len / (end - start))
    print("RX throughput:", rx_len / (end - start))
    return txcnt, seq_errors, tim_errors

def gapc_set_le_pkt_size(self, size=27, time=0):
    if self.conid is None:
        return
    if time == 0: time = (14 * 8 + size * 8)
    tskid = self.conid
    cmd = test.GAPC_SET_LE_PKT_SIZE_CMD(dst=tskid, param=(test.GAPC_SET_LE_PKT_SIZE, size, time))
    print(">>", cmd)
    print(".".join(format(o, "02X") for o in cmd.pack()))
    #cobs_encode(append_fcs(cmd.pack()))
    self.send_msg(cmd)
    resp = self.recv_msg(lambda m, c: isinstance(m, test.GAPC_CMP_EVT)
                                        and m.hdr.src == tskid
                                        and m.param.operation == test.GAPC_SET_LE_PKT_SIZE)
    print("<<", resp)
    print(".".join(format(o, "02X") for o in resp.pack()))
    return "OK" if resp.param.status == 0 else "FAILED ({})".format(hex(resp.param.status))

"""

def main():
    return 0

if __name__ == "__main__":
    main()
    