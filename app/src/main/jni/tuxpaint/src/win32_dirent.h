/****************************************************/  
/*                                                  */ 
/* For Win32 that lacks Unix direct support.        */ 
/*    - avoids including "windows.h"                */ 
/*                                                  */ 
/* Copyright (c) 2002 John Popplewell               */ 
/* john@johnnypops.demon.co.uk                      */ 
/*                                                  */ 
/****************************************************/ 
/* $Id: win32_dirent.h,v 1.3 2006/08/27 21:00:55 wkendrick Exp $ */ 
typedef long BOOL;
typedef unsigned int DWORD;
typedef wchar_t TCHAR;
typedef void *HANDLE;

#define MAX_PATH                256
#define INVALID_HANDLE_VALUE    ((HANDLE)(-1))
#define WINAPI                  __stdcall
typedef struct 
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;
typedef struct 
{
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  DWORD dwReserved0;
  DWORD dwReserved1;
  TCHAR cFileName[MAX_PATH];
  TCHAR cAlternateFileName[14];
} WIN32_FIND_DATA;

#define FindFirstFile   FindFirstFileA
#define FindNextFile    FindNextFileA
#define FindClose       FindClose
  
#ifdef __cplusplus
extern "C"
{
  
#endif	/*  */
   extern HANDLE WINAPI FindFirstFile(const char *, WIN32_FIND_DATA *);
   extern BOOL WINAPI FindNextFile(HANDLE, WIN32_FIND_DATA *);
    extern BOOL WINAPI FindClose(HANDLE);
    
#ifdef __cplusplus
};

#endif	/*  */
struct dirent 
{
  char d_name[MAX_PATH];
 };
typedef struct 
{
  WIN32_FIND_DATA wfd;
  HANDLE hFind;
  struct dirent de;
 } DIR;
extern DIR *opendir(const char *pSpec);
extern void closedir(DIR * pDir);
extern struct dirent *readdir(struct DIR *pDir);
typedef int (*selectCB) (const struct dirent *);
typedef int (*comparCB) (const void *, const void *);
extern int alphasort(const void *a, const void *b);
extern int scandir(const char *dir, struct dirent ***namelist,
		    selectCB select, comparCB compar);

