#!/usr/bin/env python3

# Ancillary wrapper around gdk-pixbuf-print-mime-types that sets up a
# modified environment in order to use the tools that we just built
# instead of the system ones

import argparse
import os
import subprocess
import sys

argparser = argparse.ArgumentParser(description='Compile resources')
argparser.add_argument('--printer', metavar='PATH', help='Path to gdk-pixbuf-print-mime-types')
argparser.add_argument('--pixdata', metavar='PATH', help='Path to gdk-pixbuf-pixdata')
argparser.add_argument('--loaders', metavar='PATH', help='Path to the loaders.cache file')
argparser.add_argument('--bindir', metavar='PATH', help='Path to the source directory')
argparser.add_argument('input', help='Template file')
argparser.add_argument('output', help='Output file')

args = argparser.parse_args()

newenv = os.environ.copy()
newenv['GDK_PIXBUF_PIXDATA'] = args.pixdata
newenv['GDK_PIXBUF_MODULE_FILE'] = args.loaders
# 'nt': NT-based Windows, see https://docs.python.org/3/library/os.html
if os.name == 'nt':
    gdk_pixbuf_dll_buildpath = os.path.dirname(args.pixdata)
    newenv['PATH'] = gdk_pixbuf_dll_buildpath + os.pathsep + newenv['PATH']

cmd = args.printer

mimetypes_out = subprocess.Popen(cmd, env=newenv, stdout=subprocess.PIPE).communicate()[0]
if not mimetypes_out:
    sys.exit(1)

infile = open(args.input, 'r')
outfile = open(args.output, 'w')

for line in infile:
    line = line.replace('@bindir@', args.bindir)
    line = line.replace('@mimetypes@', mimetypes_out.decode('ascii'))
    outfile.write(line)

infile.close()
outfile.close()

sys.exit(0)
