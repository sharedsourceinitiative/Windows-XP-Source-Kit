/*****************************************************************************\
*                                                                             *
* winver.h -    Version management functions, types, and definitions          *
*                                                                             *
*               Include file for VER.DLL.  This library is                    *
*               designed to allow version stamping of Windows executable files*
*               and of special .VER files for DOS executable files.           *
*                                                                             *
*               Copyright (c) Microsoft Corporation. All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef VER_H
#define VER_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----- Symbols ----- */
#define VS_FILE_INFO            RT_VERSION
#define VS_VERSION_INFO         1
#define VS_USER_DEFINED         100

/* ----- VS_VERSION.dwFileFlags ----- */
#ifndef _MAC
#define VS_FFI_SIGNATURE        0xFEEF04BDL
#else
#define VS_FFI_SIGNATURE        0xBD04EFFEL
#endif
#define VS_FFI_STRUCVERSION     0x00010000L
#define VS_FFI_FILEFLAGSMASK    0x0000003FL

/* ----- VS_VERSION.dwFileFlags ----- */
#define VS_FF_DEBUG             0x00000001L
#define VS_FF_PRERELEASE        0x00000002L
#define VS_FF_PATCHED           0x00000004L
#define VS_FF_PRIVATEBUILD      0x00000008L
#define VS_FF_INFOINFERRED      0x00000010L
#define VS_FF_SPECIALBUILD      0x00000020L

/* ----- VS_VERSION.dwFileOS ----- */
#define VOS_UNKNOWN             0x00000000L
#define VOS_DOS                 0x00010000L
#define VOS_OS216               0x00020000L
#define VOS_OS232               0x00030000L
#define VOS_NT                  0x00040000L

#define VOS__BASE               0x00000000L
#define VOS__WINDOWS16          0x00000001L
#define VOS__PM16               0x00000002L
#define VOS__PM32               0x00000003L
#define VOS__WINDOWS32          0x00000004L

#define VOS_DOS_WINDOWS16       0x00010001L
#define VOS_DOS_WINDOWS32       0x00010004L
#define VOS_OS216_PM16          0x00020002L
#define VOS_OS232_PM32          0x00030003L
#define VOS_NT_WINDOWS32        0x00040004L

/* ----- VS_VERSION.dwFileType ----- */
#define VFT_UNKNOWN             0x00000000L
#define VFT_APP                 0x00000001L
#define VFT_DLL                 0x00000002L
#define VFT_DRV                 0x00000003L
#define VFT_FONT                0x00000004L
#define VFT_VXD                 0x00000005L
#define VFT_STATIC_LIB          0x00000007L

/* ----- VS_VERSION.dwFileSubtype for VFT_WINDOWS_DRV ----- */
#define VFT2_UNKNOWN            0x00000000L
#define VFT2_DRV_PRINTER        0x00000001L
#define VFT2_DRV_KEYBOARD       0x00000002L
#define VFT2_DRV_LANGUAGE       0x00000003L
#define VFT2_DRV_DISPLAY        0x00000004L
#define VFT2_DRV_MOUSE          0x00000005L
#define VFT2_DRV_NETWORK        0x00000006L
#define VFT2_DRV_SYSTEM         0x00000007L
#define VFT2_DRV_INSTALLABLE    0x00000008L
#define VFT2_DRV_SOUND          0x00000009L
#define VFT2_DRV_COMM           0x0000000AL
#define VFT2_DRV_INPUTMETHOD    0x0000000BL
#define VFT2_DRV_VERSIONED_PRINTER    0x0000000CL

/* ----- VS_VERSION.dwFileSubtype for VFT_WINDOWS_FONT ----- */
#define VFT2_FONT_RASTER        0x00000001L
#define VFT2_FONT_VECTOR        0x00000002L
#define VFT2_FONT_TRUETYPE      0x00000003L

/* ----- VerFindFile() flags ----- */
#define VFFF_ISSHAREDFILE       0x0001

#define VFF_CURNEDEST           0x0001
#define VFF_FILEINUSE           0x0002
#define VFF_BUFFTOOSMALL        0x0004

/* ----- VerInstallFile() flags ----- */
#define VIFF_FORCEINSTALL       0x0001
#define VIFF_DONTDELETEOLD      0x0002

#define VIF_TEMPFILE            0x00000001L
#define VIF_MISMATCH            0x00000002L
#define VIF_SRCOLD              0x00000004L

#define VIF_DIFFLANG            0x00000008L
#define VIF_DIFFCODEPG          0x00000010L
#define VIF_DIFFTYPE            0x00000020L

#define VIF_WRITEPROT           0x00000040L
#define VIF_FILEINUSE           0x00000080L
#define VIF_OUTOFSPACE          0x00000100L
#define VIF_ACCESSVIOLATION     0x00000200L
#define VIF_SHARINGVIOLATION    0x00000400L
#define VIF_CANNOTCREATE        0x00000800L
#define VIF_CANNOTDELETE        0x00001000L
#define VIF_CANNOTRENAME        0x00002000L
#define VIF_CANNOTDELETECUR     0x00004000L
#define VIF_OUTOFMEMORY         0x00008000L

#define VIF_CANNOTREADSRC       0x00010000L
#define VIF_CANNOTREADDST       0x00020000L

#define VIF_BUFFTOOSMALL        0x00040000L
#define VIF_CANNOTLOADLZ32      0x00080000L
#define VIF_CANNOTLOADCABINET   0x00100000L

#ifndef RC_INVOKED              /* RC doesn't need to see the rest of this */

/* ----- Types and structures ----- */

typedef struct tagVS_FIXEDFILEINFO
{
    DWORD   dwSignature;            /* e.g. 0xfeef04bd */
    DWORD   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
    DWORD   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
    DWORD   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
    DWORD   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
    DWORD   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
    DWORD   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
    DWORD   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    DWORD   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
    DWORD   dwFileType;             /* e.g. VFT_DRIVER */
    DWORD   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
    DWORD   dwFileDateMS;           /* e.g. 0 */
    DWORD   dwFileDateLS;           /* e.g. 0 */
} VS_FIXEDFILEINFO;

/* ----- Function prototypes ----- */

DWORD
APIENTRY
VerFindFileA(
        DWORD uFlags,
        LPSTR szFileName,
        LPSTR szWinDir,
        LPSTR szAppDir,
        LPSTR szCurDir,
        PUINT lpuCurDirLen,
        LPSTR szDestDir,
        PUINT lpuDestDirLen
        );
DWORD
APIENTRY
VerFindFileW(
        DWORD uFlags,
        LPWSTR szFileName,
        LPWSTR szWinDir,
        LPWSTR szAppDir,
        LPWSTR szCurDir,
        PUINT lpuCurDirLen,
        LPWSTR szDestDir,
        PUINT lpuDestDirLen
        );
#ifdef UNICODE
#define VerFindFile  VerFindFileW
#else
#define VerFindFile  VerFindFileA
#endif // !UNICODE

DWORD
APIENTRY
VerInstallFileA(
        DWORD uFlags,
        LPSTR szSrcFileName,
        LPSTR szDestFileName,
        LPSTR szSrcDir,
        LPSTR szDestDir,
        LPSTR szCurDir,
        LPSTR szTmpFile,
        PUINT lpuTmpFileLen
        );
DWORD
APIENTRY
VerInstallFileW(
        DWORD uFlags,
        LPWSTR szSrcFileName,
        LPWSTR szDestFileName,
        LPWSTR szSrcDir,
        LPWSTR szDestDir,
        LPWSTR szCurDir,
        LPWSTR szTmpFile,
        PUINT lpuTmpFileLen
        );
#ifdef UNICODE
#define VerInstallFile  VerInstallFileW
#else
#define VerInstallFile  VerInstallFileA
#endif // !UNICODE

/* Returns size of version info in bytes */
DWORD
APIENTRY
GetFileVersionInfoSizeA(
        LPSTR lptstrFilename, /* Filename of version stamped file */
        LPDWORD lpdwHandle
        );                      /* Information for use by GetFileVersionInfo */
/* Returns size of version info in bytes */
DWORD
APIENTRY
GetFileVersionInfoSizeW(
        LPWSTR lptstrFilename, /* Filename of version stamped file */
        LPDWORD lpdwHandle
        );                      /* Information for use by GetFileVersionInfo */
#ifdef UNICODE
#define GetFileVersionInfoSize  GetFileVersionInfoSizeW
#else
#define GetFileVersionInfoSize  GetFileVersionInfoSizeA
#endif // !UNICODE

/* Read version info into buffer */
BOOL
APIENTRY
GetFileVersionInfoA(
        LPSTR lptstrFilename, /* Filename of version stamped file */
        DWORD dwHandle,         /* Information from GetFileVersionSize */
        DWORD dwLen,            /* Length of buffer for info */
        LPVOID lpData
        );                      /* Buffer to place the data structure */
/* Read version info into buffer */
BOOL
APIENTRY
GetFileVersionInfoW(
        LPWSTR lptstrFilename, /* Filename of version stamped file */
        DWORD dwHandle,         /* Information from GetFileVersionSize */
        DWORD dwLen,            /* Length of buffer for info */
        LPVOID lpData
        );                      /* Buffer to place the data structure */
#ifdef UNICODE
#define GetFileVersionInfo  GetFileVersionInfoW
#else
#define GetFileVersionInfo  GetFileVersionInfoA
#endif // !UNICODE

DWORD
APIENTRY
VerLanguageNameA(
        DWORD wLang,
        LPSTR szLang,
        DWORD nSize
        );
DWORD
APIENTRY
VerLanguageNameW(
        DWORD wLang,
        LPWSTR szLang,
        DWORD nSize
        );
#ifdef UNICODE
#define VerLanguageName  VerLanguageNameW
#else
#define VerLanguageName  VerLanguageNameA
#endif // !UNICODE

BOOL
APIENTRY
VerQueryValueA(
        const LPVOID pBlock,
        LPSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );
BOOL
APIENTRY
VerQueryValueW(
        const LPVOID pBlock,
        LPWSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );
#ifdef UNICODE
#define VerQueryValue  VerQueryValueW
#else
#define VerQueryValue  VerQueryValueA
#endif // !UNICODE

#endif  /* !RC_INVOKED */

#ifdef __cplusplus
}
#endif

#endif  /* !VER_H */

