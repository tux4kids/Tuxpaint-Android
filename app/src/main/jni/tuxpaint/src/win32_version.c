#include <windows.h>
#include <VersionHelpers.h>
#include <stdio.h>
#include "debug.h"

void win32_print_version(void);

void win32_print_version(void)
{
  char *verStr;
  unsigned int Version = 0;
  unsigned int Build = 0;

  Version = GetVersion();
  if (Version < 0x80000000)
    Build = (DWORD)(HIWORD(Version));

  if (IsWindows10OrGreater()) {
    if (Build < 22000){
      verStr = strdup("Windows 10");
    }else{
      verStr = strdup("Windows 11");
    }
  }else if (IsWindows8Point1OrGreater()){
    verStr = strdup("Windows 8.1");
  }else if (IsWindows8OrGreater()){
    verStr = strdup("Windows 8");
  }else if (IsWindows7SP1OrGreater()){
    verStr = strdup("Windows 7 Service Pack 1");
  }else if (IsWindows7OrGreater()){
    verStr = strdup("Windows 7");
  }else if (IsWindowsVistaSP2OrGreater()){
    verStr = strdup("Windows Vista Service Pack 2");
  }else if (IsWindowsVistaSP1OrGreater()){
    verStr = strdup("Windows Vista Service Pack 1");
  }else if (IsWindowsVistaOrGreater()){
    verStr = strdup("Windows Vista");
  }else{
    verStr = strdup("unknown");
  }

  printf ("Microsoft %s", verStr);

  if (IsWindowsServer()){
    printf(" Server");
  }

  if (Version < 0x80000000){
    printf(" (Build %d)\n", Build);
  }
}
