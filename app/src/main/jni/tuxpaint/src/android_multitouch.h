/*
  android_multitouch.h
  
  Android-specific multitouch handling via JNI
*/

#ifndef ANDROID_MULTITOUCH_H
#define ANDROID_MULTITOUCH_H

#ifdef __ANDROID__

// Initialize multitouch system
void android_multitouch_init(void);

// Get current number of active pointers
int android_multitouch_get_count(void);

// Get pointer data by index (0-based)
// Returns 1 if pointer exists, 0 if not
int android_multitouch_get_pointer(int index, long *out_id, float *out_x, float *out_y, float *out_last_x, float *out_last_y);

#endif /* __ANDROID__ */

#endif /* ANDROID_MULTITOUCH_H */
