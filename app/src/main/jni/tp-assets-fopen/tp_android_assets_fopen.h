#include <stdio.h>
#define fopen(fname, mode) android_fopen(fname, mode)


/* From SDL2 sources src/core/android/SDL_android.h */
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/* android_fopen tries first with fopen, and fallbacks to load from assets */
FILE* android_fopen(char const* fname, char const* mode);
 
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
