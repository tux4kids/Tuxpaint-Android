/*
  android_multitouch.c
  
  Android-specific multitouch handling via JNI
  Receives MotionEvents directly from Java and makes them available to tuxpaint.c
*/

#include <jni.h>
#include <android/log.h>
#include <string.h>

#define TAG "TuxPaint_Multitouch"
#define MAX_POINTERS 10

// Global state for multitouch tracking
typedef struct {
    int active;
    long pointer_id;
    float x;
    float y;
    float last_x;
    float last_y;
} AndroidPointerState;

static AndroidPointerState g_pointers[MAX_POINTERS];
static int g_pointer_count = 0;
static int g_initialized = 0;

// Initialize multitouch state
void android_multitouch_init() {
    if (!g_initialized) {
        memset(g_pointers, 0, sizeof(g_pointers));
        g_pointer_count = 0;
        g_initialized = 1;
        __android_log_print(ANDROID_LOG_INFO, TAG, "Android multitouch initialized");
    }
}

// Get current pointer count
int android_multitouch_get_count() {
    return g_pointer_count;
}

// Get pointer data by index
int android_multitouch_get_pointer(int index, long *out_id, float *out_x, float *out_y, float *out_last_x, float *out_last_y) {
    if (index < 0 || index >= MAX_POINTERS || !g_pointers[index].active) {
        return 0;
    }
    
    if (out_id) *out_id = g_pointers[index].pointer_id;
    if (out_x) *out_x = g_pointers[index].x;
    if (out_y) *out_y = g_pointers[index].y;
    if (out_last_x) *out_last_x = g_pointers[index].last_x;
    if (out_last_y) *out_last_y = g_pointers[index].last_y;
    
    return 1;
}

// JNI function called from Java
JNIEXPORT void JNICALL
Java_org_tuxpaint_tuxpaintActivity_nativeOnTouch(JNIEnv *env, jclass cls, 
                                                  jint action, jint pointerCount,
                                                  jfloatArray x, jfloatArray y, 
                                                  jlongArray pointerIds) {
    if (!g_initialized) {
        android_multitouch_init();
    }
    
    // Get array data
    jfloat *x_arr = (*env)->GetFloatArrayElements(env, x, NULL);
    jfloat *y_arr = (*env)->GetFloatArrayElements(env, y, NULL);
    jlong *id_arr = (*env)->GetLongArrayElements(env, pointerIds, NULL);
    
    // Update global state
    g_pointer_count = pointerCount;
    
    for (int i = 0; i < pointerCount && i < MAX_POINTERS; i++) {
        // Check if this is a NEW pointer (different ID or wasn't active)
        int is_new_pointer = (!g_pointers[i].active || g_pointers[i].pointer_id != id_arr[i]);
        
        g_pointers[i].active = 1;
        g_pointers[i].pointer_id = id_arr[i];
        
        if (is_new_pointer) {
            // First touch: Initialize both current AND last position to same value
            // This prevents drawing from old position to new position
            g_pointers[i].x = x_arr[i];
            g_pointers[i].y = y_arr[i];
            g_pointers[i].last_x = x_arr[i];  // Same as current position!
            g_pointers[i].last_y = y_arr[i];
        } else {
            // Continuing touch: Save previous position, then update
            g_pointers[i].last_x = g_pointers[i].x;
            g_pointers[i].last_y = g_pointers[i].y;
            g_pointers[i].x = x_arr[i];
            g_pointers[i].y = y_arr[i];
        }
    }
    
    // Clear inactive pointers
    for (int i = pointerCount; i < MAX_POINTERS; i++) {
        g_pointers[i].active = 0;
    }
    
    // Log for debugging
    static int log_counter = 0;
    if (log_counter++ % 30 == 0 || pointerCount > 1) {
        __android_log_print(ANDROID_LOG_INFO, TAG, 
            "Touch: action=%d pointers=%d", action, pointerCount);
        if (pointerCount > 1) {
            __android_log_print(ANDROID_LOG_INFO, TAG,
                "  Finger 0: (%.0f, %.0f) Finger 1: (%.0f, %.0f)",
                x_arr[0], y_arr[0], x_arr[1], y_arr[1]);
        }
    }
    
    // Release arrays
    (*env)->ReleaseFloatArrayElements(env, x, x_arr, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, y, y_arr, JNI_ABORT);
    (*env)->ReleaseLongArrayElements(env, pointerIds, id_arr, JNI_ABORT);
}
