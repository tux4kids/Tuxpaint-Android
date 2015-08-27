/* win32_print.c */
/* printing support for Tux Paint */

/* John Popplewell <john@johnnypops.demon.co.uk> */

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
*/

/* Sept. 30, 2002 - Oct. 17, 2002 */
/* Oct.  07, 2003 - added banding support */
/*                - prints using 24-bit (not 32-bit) bitmap */
/* $Id: win32_print.c,v 1.15 2008/07/10 20:57:25 johnnypops Exp $ */

#include <windows.h>
#include <direct.h>
#include "SDL_syswm.h"
#include "win32_print.h"
#include "debug.h"


#define NOREF(x)        ((x)=(x))
#define GETHINST(hWnd)	((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE))
#define MIR(id)	        (MAKEINTRESOURCE(id))

static HDC hDCprinter = NULL;

static SDL_Surface *make24bitDIB(SDL_Surface * surf)
{
  SDL_PixelFormat pixfmt;
  SDL_Surface *surf24;
  SDL_Surface *surfDIB;
  Uint8 *src, *dst;
  Uint32 linesize;
  int i;

  memset(&pixfmt, 0, sizeof(pixfmt));
  pixfmt.palette = NULL;
  pixfmt.BitsPerPixel = 24;
  pixfmt.BytesPerPixel = 3;
  pixfmt.Rmask = 0x00FF0000;
  pixfmt.Gmask = 0x0000FF00;
  pixfmt.Bmask = 0x000000FF;
  pixfmt.Amask = 0;
  pixfmt.Rshift = 16;
  pixfmt.Gshift = 8;
  pixfmt.Bshift = 0;
  pixfmt.Ashift = 0;
  pixfmt.Rloss = 0;
  pixfmt.Gloss = 0;
  pixfmt.Bloss = 0;
  pixfmt.Aloss = 0;
  pixfmt.colorkey = 0;
  pixfmt.alpha = 0;

  surf24 = SDL_ConvertSurface(surf, &pixfmt, SDL_SWSURFACE);
  surfDIB = SDL_CreateRGBSurface(SDL_SWSURFACE, surf24->w, surf24->h, 24,
				 pixfmt.Rmask, pixfmt.Gmask, pixfmt.Bmask,
				 pixfmt.Amask);

  linesize = surf24->w * 3;	// Flip top2bottom
  dst = surfDIB->pixels;
  src = ((Uint8 *) surf24->pixels) + ((surf24->h - 1) * surf24->pitch);
  for (i = 0; i < surf24->h; ++i)
  {
    memcpy(dst, src, linesize);
    src -= surf24->pitch;
    dst += surfDIB->pitch;
  }

  SDL_FreeSurface(surf24);	// Free temp surface

  return surfDIB;
}

/* returns 0 if failed */
static int GetDefaultPrinterStrings(char *device, char *driver, char *output)
{
  const char *section = "windows";
  const char *key = "device";
  const char *def = "NODEFAULTPRINTER";
  char buff[MAX_PATH];
  char *dev, *drv, *out;

  if (!GetProfileString(section, key, def, buff, sizeof(buff)))
    return 0;

  if (strcmp(buff, def) == 0)
    return 0;

  if (((dev = strtok(buff, ",")) != NULL) &&
      ((drv = strtok(NULL, ", ")) != NULL) &&
      ((out = strtok(NULL, ", ")) != NULL))
  {
    if (device)
      strcpy(device, dev);
    if (driver)
      strcpy(driver, drv);
    if (output)
      strcpy(output, out);
    return 1;
  }
  return 0;
}

#define dmDeviceNameSize    32

static HANDLE LoadCustomPrinterHDEVMODE(HWND hWnd, const char *filepath)
{
  char device[MAX_PATH];
  HANDLE hPrinter = NULL;
  int sizeof_devmode;
  HGLOBAL hDevMode = NULL;
  DEVMODE *devmode = NULL;
  int res;
  FILE *fp = NULL;
  int block_size;
  int block_read;

  if ((fp = fopen(filepath, "rb")) == NULL)
    return NULL;

  if (fread(device, 1, dmDeviceNameSize, fp) != dmDeviceNameSize)
    goto err_exit;

  if (!OpenPrinter(device, &hPrinter, NULL))
    goto err_exit;

  sizeof_devmode = (int) DocumentProperties(hWnd, hPrinter, device,
					    NULL, NULL, 0);

  if (!sizeof_devmode)
    goto err_exit;

  hDevMode = GlobalAlloc(GHND, sizeof_devmode);
  if (!hDevMode)
    goto err_exit;

  devmode = (DEVMODE *) GlobalLock(hDevMode);
  if (!devmode)
    goto err_exit;

  res = DocumentProperties(hWnd, hPrinter, device, devmode, NULL,
			   DM_OUT_BUFFER);
  if (res != IDOK)
    goto err_exit;

  block_size = devmode->dmSize + devmode->dmDriverExtra;
  block_read = fread(devmode, 1, block_size, fp);
  if (block_size != block_read)
    goto err_exit;
  fclose(fp);

  res = DocumentProperties(hWnd, hPrinter, device, devmode, devmode,
			   DM_IN_BUFFER | DM_OUT_BUFFER);
  if (res != IDOK)
    goto err_exit;

  GlobalUnlock(hDevMode);
  ClosePrinter(hPrinter);
  return hDevMode;

err_exit:
  if (fp)
    fclose(fp);
  if (devmode)
    GlobalUnlock(hDevMode);
  if (hDevMode)
    GlobalFree(hDevMode);
  if (hPrinter)
    ClosePrinter(hPrinter);
  return NULL;
}


static int SaveCustomPrinterHDEVMODE(HWND hWnd, const char *filepath,
				     HANDLE hDevMode)
{
  FILE *fp = NULL;

  NOREF(hWnd);
  if ((fp = fopen(filepath, "wb")) != NULL)
  {
    DEVMODE *devmode = (DEVMODE *) GlobalLock(hDevMode);
    int block_size = devmode->dmSize + devmode->dmDriverExtra;
    int block_written;
    char devname[dmDeviceNameSize];

    strcpy(devname, (const char *) devmode->dmDeviceName);
    fwrite(devname, 1, sizeof(devname), fp);
    block_written = fwrite(devmode, 1, block_size, fp);
    GlobalUnlock(hDevMode);
    fclose(fp);
    return block_size == block_written;
  }
  return 0;
}

static int FileExists(const char *filepath)
{
    FILE *fp;

    if ((fp = fopen(filepath, "rb")) != NULL)
    {
      fclose(fp);
      return 1;
    }
    return 0;
}

static int GetCustomPrinterDC(HWND hWnd, const char *printcfg, int show)
{
  PRINTDLG pd = {
    sizeof(PRINTDLG),
    hWnd, NULL, NULL, NULL,
    PD_RETURNDC,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    1,
    0, 0, 0, 0, 0, 0, 0, 0,
  };

  pd.hDevMode = LoadCustomPrinterHDEVMODE(hWnd, printcfg);

  if (show || !FileExists(printcfg))
  {
    if (PrintDlg(&pd))
    {
      hDCprinter = pd.hDC;
      SaveCustomPrinterHDEVMODE(hWnd, printcfg, pd.hDevMode);
      GlobalFree(pd.hDevMode);
      return 1;
    }
    GlobalFree(pd.hDevMode);
    return 0;
  }

  {
    DEVMODE *devmode = (DEVMODE *) GlobalLock(pd.hDevMode);

    hDCprinter =
      CreateDC(NULL, (const char *) devmode->dmDeviceName, NULL, devmode);
    GlobalUnlock(pd.hDevMode);
    GlobalFree(pd.hDevMode);
  }
  return 1;
}


static HDC GetDefaultPrinterDC(void)
{
  char device[MAX_PATH], driver[MAX_PATH], output[MAX_PATH];

  if (GetDefaultPrinterStrings(device, driver, output))
    return CreateDC(driver, device, output, NULL);

  return NULL;
}

static int GetPrinterDC(HWND hWnd, const char *printcfg, int show)
{
  hDCprinter = NULL;

  if (printcfg)
  {
    return GetCustomPrinterDC(hWnd, printcfg, show);
  }
  hDCprinter = GetDefaultPrinterDC();
  return 1;
}


int IsPrinterAvailable(void)
{
  return (GetDefaultPrinterStrings(NULL, NULL, NULL) != 0);
}

#define STRETCH_TO_FIT  0
#define SCALE_TO_FIT    1

const char *SurfacePrint(SDL_Surface * surf, const char *printcfg,
			 int showdialog)
{
  const char *res = NULL;
  HWND hWnd;
  DOCINFO di;
  int nError;
  SDL_SysWMinfo wminfo;
  BITMAPINFOHEADER bmih;
  SDL_Surface *surf24 = NULL;
  RECT rcDst;
  float sX, sY;
  int pageWidth, pageHeight;
  int hDCCaps;
  HBITMAP hbm = NULL;
  HDC hdcMem = NULL;
  int scaling = SCALE_TO_FIT;

  SDL_VERSION(&wminfo.version);
  if (!SDL_GetWMInfo(&wminfo))
    return "win32_print: SDL_GetWMInfo() failed.";

  hWnd = wminfo.window;
  if (!GetPrinterDC(hWnd, printcfg, showdialog))
  {
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return NULL;
  }

  if (!hDCprinter)
    return "win32_print: GetPrinterDC() failed.";

  EnableWindow(hWnd, FALSE);

  di.cbSize = sizeof(DOCINFO);
  di.lpszDocName = "Tux Paint";
  di.lpszOutput = (LPTSTR) NULL;
  di.lpszDatatype = (LPTSTR) NULL;
  di.fwType = 0;

  nError = StartDoc(hDCprinter, &di);
  if (nError == SP_ERROR)
  {
    res = "win32_print: StartDoc() failed.";
    goto error;
  }

  nError = StartPage(hDCprinter);
  if (nError <= 0)
  {
    res = "win32_print: StartPage() failed.";
    goto error;
  }

//////////////////////////////////////////////////////////////////////////////////////

  surf24 = make24bitDIB(surf);
  if (!surf24)
  {
    res = "win32_print: make24bitDIB() failed.";
    goto error;
  }

  memset(&bmih, 0, sizeof(bmih));
  bmih.biSize = sizeof(bmih);
  bmih.biPlanes = 1;
  bmih.biCompression = BI_RGB;
  bmih.biBitCount = 24;
  bmih.biWidth = surf24->w;
  bmih.biHeight = surf24->h;

  pageWidth  = GetDeviceCaps(hDCprinter, HORZRES);
  pageHeight = GetDeviceCaps(hDCprinter, VERTRES);
  sX  = GetDeviceCaps(hDCprinter, LOGPIXELSX);
  sY  = GetDeviceCaps(hDCprinter, LOGPIXELSY);

  switch (scaling)
  {
    case STRETCH_TO_FIT:
    {
        /* stretches x and y dimensions independently to fit the page */
        /* doesn't preserve image aspect-ratio */
        rcDst.top = 0; rcDst.left = 0;
        rcDst.bottom = pageHeight; rcDst.right = pageWidth;
        break;
    }
    case SCALE_TO_FIT:
    {
        /* maximises image size on the page */
        /* preserves aspect-ratio, alignment is top and center */
        int width  = bmih.biWidth;
        int height = bmih.biHeight;

        if (width < pageWidth && height < pageHeight)
        {
            float   dW = (float)pageWidth  / width;
            float   dH = (float)pageHeight / height;

            if (dW < dH)
            {
                width  = pageWidth;
                height = (int)((height * dW * (sY/sX)) + 0.5f);
            }
            else
            {
                width  = (int)((width  * dH * (sX/sY)) + 0.5f);
                height = pageHeight;
            }
        }
        if (width > pageWidth)
        {
            height= height*width/pageWidth;
            width = pageWidth;
        }
        if (height > pageHeight)
        {
            width= width*height/pageHeight;
            height = pageHeight;
        }

        rcDst.top = 0;
        rcDst.left = (pageWidth-width)/2;
        rcDst.bottom = rcDst.top+height;
        rcDst.right  = rcDst.left+width;
        break;
    }
    default:
        res = "win32_print: invalid scaling option.";
        goto error;
  }

  hDCCaps = GetDeviceCaps(hDCprinter, RASTERCAPS);

  if (hDCCaps & RC_PALETTE)
  {
    res = "win32_print: printer context requires palette.";
    goto error;
  }

  if (hDCCaps & RC_STRETCHDIB)
  {
    SetStretchBltMode(hDCprinter, COLORONCOLOR);

    nError = StretchDIBits(hDCprinter, rcDst.left, rcDst.top,
      		     rcDst.right  - rcDst.left,
                           rcDst.bottom - rcDst.top,
      		     0, 0, bmih.biWidth, bmih.biHeight,
      		     surf24->pixels, (BITMAPINFO *) & bmih,
      		     DIB_RGB_COLORS, SRCCOPY);
    if (nError == GDI_ERROR)
    {
      res = "win32_print: StretchDIBits() failed.";
      goto error;
    }
  }
  else
  {
    res = "win32_print: StretchDIBits() not available.";
    goto error;
  }

//////////////////////////////////////////////////////////////////////////////////////

  nError = EndPage(hDCprinter);
  if (nError <= 0)
  {
    res = "win32_print: EndPage() failed.";
    goto error;
  }

  EndDoc(hDCprinter);

error:
  if (hdcMem)
    DeleteDC(hdcMem);
  if (hbm)
    DeleteObject(hbm);
  if (surf24)
    SDL_FreeSurface(surf24);

  EnableWindow(hWnd, TRUE);
  ShowWindow(hWnd, SW_SHOWNORMAL);
  DeleteDC(hDCprinter);

  return res;
}

/*
  Read access to Windows Registry
*/
static HRESULT ReadRegistry(const char *key, const char *option, char *value,
			    int size)
{
  LONG res;
  HKEY hKey = NULL;

  res = RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey);
  if (res != ERROR_SUCCESS)
    goto err_exit;
  res =
    RegQueryValueEx(hKey, option, NULL, NULL, (LPBYTE) value,
		    (LPDWORD) & size);
  if (res != ERROR_SUCCESS)
    goto err_exit;
  res = ERROR_SUCCESS;

err_exit:
  if (hKey)
    RegCloseKey(hKey);
  return HRESULT_FROM_WIN32(res);
}

/*
  Removes a single '\' or '/' from end of path 
*/
static char *remove_slash(char *path)
{
  int len = strlen(path);

  if (!len)
    return path;

  if (path[len - 1] == '/' || path[len - 1] == '\\')
    path[len - 1] = 0;

  return path;
}

/*
  Returns heap string containing default application data path.
  Creates suffix subdirectory (only one level).
  E.g. C:\Documents and Settings\jfp\Application Data\suffix
*/
char *GetDefaultSaveDir(const char *suffix)
{
  char prefix[MAX_PATH];
  char path[2 * MAX_PATH];
  const char *key =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
  const char *option = "AppData";
  HRESULT hr = S_OK;

  if (SUCCEEDED(hr = ReadRegistry(key, option, prefix, sizeof(prefix))))
  {
    remove_slash(prefix);
    snprintf(path, sizeof(path), "%s/%s", prefix, suffix);
    _mkdir(path);
    return strdup(path);
  }
  return strdup("userdata");
}

/*
  Returns heap string containing system font directory.
  E.g. 'C:\Windows\Fonts'
*/
char *GetSystemFontDir(void)
{
  char path[MAX_PATH];
  const char *key =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
  const char *option = "Fonts";
  HRESULT hr = S_OK;

  if (SUCCEEDED(hr = ReadRegistry(key, option, path, sizeof(path))))
  {
    remove_slash(path);
    return strdup(path);
  }
  return strdup("C:\\WINDOWS\\FONTS");
}

/*
  Returns heap string containing user temp directory.
  E.g. C:\Documents and Settings\jfp\Local Settings\Temp
*/
static char *GetUserTempDir(void)
{
  char *temp = getenv("TEMP");

  if (!temp)
  {
    temp = "userdata";
  }
  return strdup(temp);
}

char *get_temp_fname(const char *const name)
{
  char f[512];
  char *tempdir = GetUserTempDir();

  snprintf(f, sizeof(f), "%s/%s", tempdir, name);
  free(tempdir);
  return strdup(f);
}

/*
 * Nasty low-level hook into the keyboard. 2K/XP/Vista only.
 */

static HHOOK g_hKeyboardHook = NULL;
static int   g_bWindowActive = 0;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  int bEatKeystroke = 0;
  KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT*)lParam;

  if (nCode < 0 || nCode != HC_ACTION)
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam); 
 
  switch (wParam) 
  {
    case WM_KEYDOWN:  
    case WM_KEYUP:    
    {
      bEatKeystroke = g_bWindowActive && ((p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN));
      break;
    }
  }
 
  if(bEatKeystroke)
    return 1;
  return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

int InstallKeyboardHook(void)
{
  if (g_hKeyboardHook)
    return -1;
  g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
  return g_hKeyboardHook ? 0 : -2;
}

int RemoveKeyboardHook(void)
{
  if (!g_hKeyboardHook)
    return -1;
  UnhookWindowsHookEx(g_hKeyboardHook);
  g_hKeyboardHook = NULL;
  return 0;
}

void SetActivationState(int state)
{
  g_bWindowActive = state;
}


