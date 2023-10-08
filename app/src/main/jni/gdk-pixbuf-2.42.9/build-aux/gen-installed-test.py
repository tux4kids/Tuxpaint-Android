#!/usr/bin/env python3

import sys
import os
import argparse

def write_template(filename, data):
    with open(filename, 'w') as f:
        f.write(data)

def build_template(bindir, binname):
    return "[Test]\nType=session\nExec={}\n".format(os.path.join(bindir, binname))

argparser = argparse.ArgumentParser(description='Generate installed-test data.')
argparser.add_argument('--testbindir', metavar='dir', help='Installed test directory')
argparser.add_argument('--testbin', metavar='name', help='Installed test name')
argparser.add_argument('output', help='Output file')

args = argparser.parse_args()

write_template(args.output, build_template(args.testbindir, args.testbin))
