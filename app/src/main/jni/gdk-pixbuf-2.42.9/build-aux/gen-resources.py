#!/usr/bin/env python3

# Ancillary wrapper around glib-compile-resources that sets up a
# modified environment in order to use the tools that we just
# built instead of the system ones

import argparse
import os

argparser = argparse.ArgumentParser(description='Compile resources')
argparser.add_argument('--glib-compile-resources', metavar='PATH', help='Path to glib-compile-resources')
argparser.add_argument('--pixdata', metavar='PATH', help='Path to gdk-pixbuf-pixdata')
argparser.add_argument('--loaders', metavar='PATH', help='Path to the loaders.cache file')
argparser.add_argument('--sourcedir', metavar='PATH', help='Path to the source directory')
argparser.add_argument('resource', help='GResource XML file')
argparser.add_argument('output', help='Output file')

group = argparser.add_mutually_exclusive_group()
group.add_argument('--header', help='Generate header file', action='store_true')
group.add_argument('--source', help='Generate source file', action='store_true')

args = argparser.parse_args()

cmd = []

if args.glib_compile_resources:
    cmd += [args.glib_compile_resources]
else:
    cmd += ['glib-compile-resources']

if args.header:
    cmd += ['--generate-header']
else:
    cmd += ['--generate-source']

cmd += ['--sourcedir', args.sourcedir]
cmd += [args.resource]
cmd += ['--target', args.output]

newenv = os.environ.copy()
newenv['GDK_PIXBUF_PIXDATA'] = args.pixdata
newenv['GDK_PIXBUF_MODULE_FILE'] = args.loaders

os.execvpe(cmd[0], cmd, newenv)
