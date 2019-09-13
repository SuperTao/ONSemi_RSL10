#!/usr/bin/env python

WHITELIST = {
    # symols are defined by fota_stack_protocol (adapted from rsl10_protocol)
    "ble_deviceParam", "Device_Param_Read", "BLE_DeviceParam_Set_MaxRxOctet", "BLE_DeviceParam_Set_AdvDelay",
    "BLE_DeviceParam_Set_SlaveLatencyDelay",
    
    # symols are defined by libgcc.a and are (hopefully) side effect free
    "__aeabi_dadd", "__aeabi_dsub", "__aeabi_dmul", "__aeabi_fmul",
    "__aeabi_d2f", "__aeabi_d2uiz", "__aeabi_f2d", "__aeabi_f2iz", "__aeabi_i2d",
    "__aeabi_ldivmod", "__aeabi_uldivmod",
    
    # symols are defined by libg_nano.a and are (hopefully) side effect free
    "memcmp", "memcpy", "memset"
}

def check(file):
    undef = []
    for line in file:
        cls, name = line.split()
        if not (name.startswith("__wrap_") or name in WHITELIST):
            undef.append(name)
    return undef


if __name__ == "__main__":
    
    import sys
    
    undef = check(sys.stdin)
    if undef:
        print("Found unknown undefined symbols in fota_stack.o:")
        print("\n".join("    {}".format(s) for s in undef))
        print("\nTo correct this add matching wrappers to fota_wrapper.c"
              "\nor if the symbol refers to a side effect free function"
              "\nadd it to the WHITELIST in check.py.\n")
        sys.exit(1)
