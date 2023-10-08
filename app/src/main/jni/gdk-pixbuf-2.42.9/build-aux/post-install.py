#!/usr/bin/env python3

import os
import sys
import subprocess

if 'DESTDIR' not in os.environ:
    bindir = sys.argv[1]
    libdir = sys.argv[2]
    binary_version = sys.argv[3]

    query_loaders = os.path.join(bindir, "gdk-pixbuf-query-loaders")
    loaders_dir = os.path.join(libdir, "gdk-pixbuf-2.0", binary_version)
    loaders_cache = os.path.join(loaders_dir, "loaders.cache")

    os.makedirs(loaders_dir, exist_ok=True)

    cmd = [query_loaders]
    with subprocess.Popen(cmd, stdout=subprocess.PIPE, close_fds=True) as p:
        data = p.stdout.read()

    with open(loaders_cache, "wb") as f:
        f.write(data)
else:
    print("*** Warning: loaders.cache not built because DESTDIR is set")
    print("***          You will need to manually call gdk-pixbuf-query-loaders")
