#!/usr/bin/env python

from collections import OrderedDict

NIST256P_SIGNATURE_SIZE = 64
INCLUDE_SYMBOLS = {
    "__text_start__", "__data_start__", "__image_size"
}
INCLUDE_SECTIONS = {
    ".text_stack", ".data_stack", ".bss_stack", ".noinit_stack"
}

def valid_symbol(name, section):
    if name in INCLUDE_SYMBOLS:
        return True
    if name.startswith("__"):
        return False
    if section in INCLUDE_SECTIONS:
        return True
    return False


def extract_symbols(file):
    symbols = OrderedDict()
    lines = file.readlines()
    for line in lines[6:]:
        name, adr, cls, type, size, line, section = (s.strip() for s in line.split("|"))
        if valid_symbol(name, section):
            adr = int(adr, base=16)
            # for function symbals we have to add 1 to the address (thumb mode)!
            if type == "FUNC":
                adr += 1
            symbols[name] = adr
    
    text_start = symbols.pop("__text_start__")
    data_start = symbols.pop("__data_start__")
    image_size = symbols.pop("__image_size")
    app_rom_start = text_start + image_size + NIST256P_SIGNATURE_SIZE
    app_rom_start += -app_rom_start % 0x800
    symbols["__app_rom_start"] = app_rom_start
    symbols["__app_ram_start"] = data_start
    
    FMT = "--add-symbol {name}=0x{adr:08x}"
    text = "\n".join(FMT.format(name=name, adr=adr) for name, adr in symbols.items())
    
    return text;



if __name__ == "__main__":
    
    import sys
    
    print(extract_symbols(sys.stdin))
