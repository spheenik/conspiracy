#ifndef CONSPIRACY_TYPES_H
#define CONSPIRACY_TYPES_H

typedef unsigned char BYTE, *PBYTE, *LPBYTE;
typedef unsigned short WORD, *PWORD, *LPWORD;
typedef int INT, *LPINT;
typedef unsigned int UINT;
typedef unsigned long LONG;

typedef char* PSTR, *LPSTR;
typedef unsigned long DWORD, *PDWORD, *LPDWORD;
typedef unsigned long DWORD, *PDWORD, *LPDWORD, *DWORD_PTR, *LRESULT;
typedef int BOOL, *PBOOL, *LPBOOL;
typedef void* HANDLE;
typedef void* LPVOID;
typedef char* LPCTSTR;
typedef unsigned int WPARAM;
typedef unsigned int LPARAM;

#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name

#endif //CONSPIRACY_TYPES_H
