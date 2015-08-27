#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

#
# To update:
#

#  git remote add libxml2 git://git.gnome.org/libxml2
#  git fetch libxml2
#  git merge libxml2/master
#  mm -j32
#  # (Make any necessary Android.mk changes and test the new libxml2.)
#  git push aosp HEAD:master  # Push directly, avoiding gerrit.
#  git push aosp HEAD:refs/for/master  # Push to gerrit.
#
#  # Now commit any necessary Android.mk changes like normal:
#  repo start post-sync .
#  git commit -a
#

# This comes from the automake-generated Makefile.
# We deliberately exclude nanoftp.c and nanohttp.c, the trio library, and zlib.
common_SRC_FILES := SAX.c entities.c encoding.c error.c \
        parserInternals.c parser.c tree.c hash.c list.c xmlIO.c \
        xmlmemory.c uri.c valid.c xlink.c HTMLparser.c HTMLtree.c \
        debugXML.c xpath.c xpointer.c xinclude.c \
        DOCBparser.c catalog.c globals.c threads.c c14n.c xmlstring.c \
        buf.c xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c \
        xmlreader.c relaxng.c dict.c SAX2.c \
        xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c xmlmodule.c \
        schematron.c

#common_C_INCLUDES += $(LOCAL_PATH)/include
common_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)

common_EXPORT_C_INCLUDES += $(LOCAL_PATH)/include

common_CFLAGS += -DLIBXML_THREAD_ENABLED=1

common_CFLAGS += \
    -Wno-missing-field-initializers \
    -Wno-self-assign \
    -Wno-sign-compare \
    -Wno-tautological-pointer-compare \

# Static library
#=======================================================

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := $(common_SRC_FILES)
#LOCAL_C_INCLUDES += $(common_C_INCLUDES)
#LOCAL_CFLAGS += $(common_CFLAGS) -fvisibility=hidden
#LOCAL_SHARED_LIBRARIES += libicuuc
#LOCAL_MODULE := libxml2
#LOCAL_CLANG := true
#LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.mk
#include $(BUILD_STATIC_LIBRARY)

# Shared library
#=======================================================

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(common_EXPORT_C_INCLUDES)
LOCAL_CFLAGS += $(common_CFLAGS)
#LOCAL_SHARED_LIBRARIES := libicuuc
#LOCAL_MODULE:= libxml2
LOCAL_SHARED_LIBRARIES := tuxpaint_iconv
LOCAL_MODULE:= tuxpaint_xml2
LOCAL_CLANG := true
LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.mk
include $(BUILD_SHARED_LIBRARY)

# For the host
# ========================================================

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := $(common_SRC_FILES)
#LOCAL_C_INCLUDES += $(common_C_INCLUDES)
#LOCAL_CFLAGS += $(common_CFLAGS) -fvisibility=hidden
#LOCAL_SHARED_LIBRARIES += libicuuc-host
#LOCAL_MODULE := libxml2
#LOCAL_CLANG := true
#LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.mk
#include $(BUILD_HOST_STATIC_LIBRARY)
