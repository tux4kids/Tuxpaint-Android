


#include "rsvg.h"

/* enumerations from "rsvg.h" */
GType
rsvg_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { RSVG_ERROR_FAILED, "RSVG_ERROR_FAILED", "failed" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("RsvgError", values);
  }
  return etype;
}
GType
rsvg_handle_flags_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { RSVG_HANDLE_FLAGS_NONE, "RSVG_HANDLE_FLAGS_NONE", "flags-none" },
      { RSVG_HANDLE_FLAG_UNLIMITED, "RSVG_HANDLE_FLAG_UNLIMITED", "flag-unlimited" },
      { RSVG_HANDLE_FLAG_KEEP_IMAGE_DATA, "RSVG_HANDLE_FLAG_KEEP_IMAGE_DATA", "flag-keep-image-data" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("RsvgHandleFlags", values);
  }
  return etype;
}



