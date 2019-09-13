#!/usr/bin/env python

import re
from sys import version_info

if version_info < (3, ):
    flags = re.MULTILINE 
else:
    flags = re.MULTILINE | re.ASCII


def wrap(file):
    WRAP = "__wrap_"
    for name in re.findall(r"__wrap_(\w+)", file.read(), flags):
        print("--wrap={}".format(name))


if __name__ == "__main__":
    
    import sys
    
    wrap(sys.stdin)
