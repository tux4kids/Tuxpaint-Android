#define DEBUG
#define VERBOSE

/*
* Enable verbose logging if requested on platforms that support it.
*
* Verbose logging adds metadata to printf, including the source file location
* from where printf was called and the time it was called at runtime.
*/
#if defined(DEBUG) && defined(VERBOSE) && defined(__GNUC__)
#include <stdio.h>
#include <time.h>

#define printf(args...) do { \
    time_t now = time(NULL); \
    printf("\n### %s, line %d in %s() @ %s", __FILE__, __LINE__, __FUNCTION__, ctime(&now)); \
    printf(args); \
} while(0)
#endif


/*
* Define a convenience macro DEBUG_PRINTF().  This macro resolves to printf()
* if and only if DEBUG is enabled, otherwise resolves to nothing.  In other
* words,
*
*   DEBUG_PRINTF("Hello, world!\n");
*
* ... is equivalent to:
*
*   #if defined(DEBUG)
*   printf("Hello, world!\n");
*   #endif
*
* (To be precise, the semicolon falls outside of the #if test, but an empty
* semicolon resolves to nothing in standard C.)
*
* If VERBOSE logging is enabled, DEBUG_PRINTF should resolve to the verbose
* version of printf() defined earlier in this file.
*/
#if defined(DEBUG)
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#ifdef __ANDROID__
#include <android/log.h>
#define  LOG_TAG    "TuxPaint"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif
