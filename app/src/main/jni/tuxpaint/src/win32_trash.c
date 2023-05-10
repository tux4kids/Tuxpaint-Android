#include <windows.h>
#include <tchar.h>

int MoveFileToRecycleBin(const TCHAR * fullPathName);
int win32_trash(const char *path);

int MoveFileToRecycleBin(const TCHAR * fullPathName)
{
  SHFILEOPSTRUCT fileOp;
  const TCHAR *src = fullPathName;
  TCHAR *dest;

  fileOp.pFrom = dest = alloca(sizeof(*dest) * (_tcslen(fullPathName) + 2));
  while ((*dest++ = *src++) != _T('\0'))
  {
  }
  *dest = _T('\0');

  fileOp.hwnd = NULL;
  fileOp.wFunc = FO_DELETE;
  fileOp.pTo = NULL;
  fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | FOF_ALLOWUNDO;
  return SHFileOperation(&fileOp);
}

int win32_trash(const char *path)
{
  char *p, *src;

  src = p = strdup(path);
  while (*p != '\0')
  {
    if (*p == '/')
      *p = '\\';
    p++;
  }
  return MoveFileToRecycleBin(src);
}
