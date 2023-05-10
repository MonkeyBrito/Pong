#pragma once

#include <windows.h>

#define IDI_OWL 101

#define VER_FILEVERSION           1,0,0,0
#define VER_FILEVERSION_STR      "1,0,0,0"
#define VER_FILEDESCRIPTION_STR  "Owl Game Engine"
#define VER_PRODUCTVERSION        1,0,0,0
#define VER_PRODUCTVERSION_STR   "1,0,0,0"
#define VER_PRODUCTNAME_STR      "Owl Game Engine"
#define VER_COMPANYNAME_STR      "MonkeyBrito"
#define VER_INTERNALNAME_STR     "Owl"
#define VER_LEGALCOPYRIGHT_STR   "Copyright(C)2023 MonkeyBrito"
// #define VER_LEGALTRADEMARKS1_STR
// #define VER_LEGALTRADEMARKS2_STR

#ifdef _DEBUG
    #define VER_DEBUG VS_FF_DEBUG | VS_FF_PRERELEASE
#else
    #define VER_DEBUG 0
#endif

#ifdef _DLL
    #define VER_TYPE VFT_DLL
    #define VER_ORIGINALFILENAME_STR "Owl.dll"
#else
    #define VER_TYPE VFT_APP
    #define VER_ORIGINALFILENAME_STR "Owl.exe"
#endif