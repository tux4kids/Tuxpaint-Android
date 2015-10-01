#define DEBUG

#ifdef __ANDROID__
#include <android/log.h>
#define  LOG_TAG    "TuxPaint"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif
