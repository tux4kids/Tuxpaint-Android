/* android_print.cpp */

/* printing support for Tux Paint */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  (See COPYING.txt)

*/
  
// Based on JNI and PrintHelper class
// https://developer.android.com/reference/android/support/v4/print/PrintHelper.html
// https://developer.android.com/training/printing/photos.html

#include "android_print.h"
#include "jni.h"

// Since Print work is based on Java PrintHelper class, which may not be supported on some old versions
int IsPrinterAvailable( void )
{
	JNIEnv *mEnv = Android_JNI_GetEnv();
        jclass mPrintHelperClass = (*mEnv)->FindClass(mEnv, "android/support/v4/print/PrintHelper");

	if (mPrintHelperClass == NULL)
	return 0;

	jmethodID mSupportMethod = (*mEnv)->GetStaticMethodID(mEnv, mPrintHelperClass, "systemSupportsPrint", "()Z");
    	jboolean support = (*mEnv)->CallStaticBooleanMethod(mEnv, mPrintHelperClass, mSupportMethod);

	return support ? 1 : 0;
}

// This function is based on 
// (1) convert surface to Java BitMap object
// (2) call Java PrintHelper to do print job.
const char *SurfacePrint(SDL_Surface *surface)
{
	JNIEnv *mEnv = Android_JNI_GetEnv();
	jclass mBitmapClass = (*mEnv)->FindClass(mEnv, "android/graphics/Bitmap");
	jmethodID mCreateMethod = (*mEnv)->GetStaticMethodID(mEnv, mBitmapClass, "createBitmap", "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
	jintArray mSurfaceArray = (*mEnv)->NewIntArray(mEnv, surface->w * surface->h);
	(*mEnv)->SetIntArrayRegion(mEnv,mSurfaceArray, 0, surface->w * surface->h, surface->pixels);
	jclass mConfigClass = (*mEnv)->FindClass(mEnv, "android/graphics/Bitmap$Config");
	jfieldID mConfigField = (*mEnv)->GetStaticFieldID(mEnv, mConfigClass , "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
	jobject mConfig = (*mEnv)->GetStaticObjectField(mEnv, mConfigClass, mConfigField);
	jobject mBitMap = (*mEnv)->CallStaticObjectMethod(mEnv, mBitmapClass, mCreateMethod, mSurfaceArray, surface->w, surface->h, mConfig);

	jobject mContext = (jobject)SDL_AndroidGetActivity();
	jclass mPrintClass = (*mEnv)->FindClass(mEnv, "android/support/v4/print/PrintHelper");
	// sometimes android v4 support library may be not ready
	if (mPrintClass == NULL)
	return "There is no android v4 support library.";
	jmethodID mInitMethod = (*mEnv)->GetMethodID(mEnv, mPrintClass, "<init>", "(Landroid/content/Context;)V");
	jobject mPrint = (*mEnv)->NewObject(mEnv, mPrintClass, mInitMethod, mContext);
	jmethodID mPrintMethod = (*mEnv)->GetMethodID(mEnv, mPrintClass, "printBitmap", "(Ljava/lang/String;Landroid/graphics/Bitmap;)V");
	jstring mString = (*mEnv)->NewStringUTF(mEnv, "TuxPaint");
	(*mEnv)->CallVoidMethod(mEnv, mPrint, mPrintMethod, mString, mBitMap);

	// clean up
	(*mEnv)->DeleteLocalRef(mEnv, mSurfaceArray);
	(*mEnv)->DeleteLocalRef(mEnv, mConfig);
	(*mEnv)->DeleteLocalRef(mEnv, mPrint);
    (*mEnv)->DeleteLocalRef(mEnv, mString);
	return NULL;
}
