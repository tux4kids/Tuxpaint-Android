#!/usr/bin/env python3

import os
import shutil

references = [
    'docs/gdk-pixbuf',
    'docs/gdk-pixdata',
]

sourceroot = os.environ.get('MESON_SOURCE_ROOT')
buildroot = os.environ.get('MESON_BUILD_ROOT')
distroot = os.environ.get('MESON_DIST_ROOT')

for reference in references:
    src_path = os.path.join(buildroot, reference)
    if os.path.exists(src_path):
        dst_path = os.path.join(distroot, reference)
        if os.path.isdir(src_path):
            shutil.copytree(src_path, dst_path)
        elif os.path.isfile(src_path):
            shutil.copyfile(src_path, dst_path)
