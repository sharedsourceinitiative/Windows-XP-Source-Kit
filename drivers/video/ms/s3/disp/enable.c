/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// Useful for visualizing the off-screen heap when set to '1':

#define DEBUG_HEAP 0

/******************************Public*Structure****************************\
* GDIINFO ggdiDefault
*
* This contains the default GDIINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

GDIINFO ggdiDefault = {
    GDI_DRIVER_VERSION,
    DT_RASDISPLAY,          // ulTechnology
    0,                      // ulHorzSize (filled in later)
    0,                      // ulVertSize (filled in later)
    0,                      // ulHorzRes (filled in later)
    0,                      // ulVertRes (filled in later)
    0,                      // cBitsPixel (filled in later)
    0,                      // cPlanes (filled in later)
    20,                     // ulNumColors (palette managed)
    0,                      // flRaster (DDI reserved field)

    0,                      // ulLogPixelsX (filled in later)
    0,                      // ulLogPixelsY (filled in later)

    TC_RA_ABLE,             // flTextCaps -- If we had wanted console windows
                            //   to scroll by repainting the entire window,
                            //   instead of doing a screen-to-screen blt, we
                            //   would have set TC_SCROLLBLT (yes, the flag is
                            //   bass-ackwards).

    0,                      // ulDACRed (filled in later)
    0,                      // ulDACGreen (filled in later)
    0,                      // ulDACBlue (filled in later)

    0x0024,                 // ulAspectX
    0x0024,                 // ulAspectY
    0x0033,                 // ulAspectXY (one-to-one aspect ratio)

    1,                      // xStyleStep
    1,                      // yStyleSte;
    3,                      // denStyleStep -- Styles have a one-to-one aspect
                            //   ratio, and every 'dot' is 3 pixels long

    { 0, 0 },               // ptlPhysOffset
    { 0, 0 },               // szlPhysSize

    256,                    // ulNumPalReg

    // These fields are for halftone initialization.  The actual values are
    // a bit magic, but seem to work well on our display.

    {                       // ciDevice
       { 6700, 3300, 0 },   //      Red
       { 2100, 7100, 0 },   //      Green
       { 1400,  800, 0 },   //      Blue
       { 1750, 3950, 0 },   //      Cyan
       { 4050, 2050, 0 },   //      Magenta
       { 4400, 5200, 0 },   //      Yellow
       { 3127, 3290, 0 },   //      AlignmentWhite
       20000,               //      RedGamma
       20000,               //      GreenGamma
       20000,               //      BlueGamma
       0, 0, 0, 0, 0, 0     //      No dye correction for raster displays
    },

    0,                       // ulDevicePelsDPI (for printers only)
    PRIMARY_ORDER_CBA,       // ulPrimaryOrder
    HT_PATSIZE_4x4_M,        // ulHTPatternSize
    HT_FORMAT_8BPP,          // ulHTOutputFormat
    HT_FLAG_ADDITIVE_PRIMS,  // flHTFlags
    0,                       // ulVRefresh
    0,                       // ulPanningHorzRes
    0,                       // ulPanningVertRes
    0,                       // ulBltAlignment
};

/******************************Public*Structure****************************\
* DEVINFO gdevinfoDefault
*
* This contains the default DEVINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO gdevinfoDefault = {
    (GCAPS_OPAQUERECT       |
     GCAPS_DITHERONREALIZE  |
     GCAPS_PALMANAGED       |
     GCAPS_ALTERNATEFILL    |
     GCAPS_WINDINGFILL      |
     GCAPS_MONO_DITHER      |
     GCAPS_COLOR_DITHER     |
     GCAPS_DIRECTDRAW       |
     GCAPS_ASYNCMOVE),          // NOTE: Only enable ASYNCMOVE if your code
                                //   and hardware can handle DrvMovePointer
                                //   calls at any time, even while another
                                //   thread is in the middle of a drawing
                                //   call such as DrvBitBlt.

                                                // flGraphicsFlags
    SYSTM_LOGFONT,                              // lfDefaultFont
    HELVE_LOGFONT,                              // lfAnsiVarFont
    COURI_LOGFONT,                              // lfAnsiFixFont
    0,                                          // cFonts
    BMF_8BPP,                                   // iDitherFormat
    8,                                          // cxDither
    8,                                          // cyDither
    0                                           // hpalDefault (filled in later)
};

/******************************Public*Structure****************************\
* DFVFN gadrvfn[]
*
* Build the driver function table gadrvfn with function index/address
* pairs.  This table tells GDI which DDI calls we support, and their
* location (GDI does an indirect call through this table to call us).
*
* Why haven't we implemented DrvSaveScreenBits?  To save code.
*
* When the driver doesn't hook DrvSaveScreenBits, USER simulates on-
* the-fly by creating a temporary device-format-bitmap, and explicitly
* calling DrvCopyBits to save/restore the bits.  Since we already hook
* DrvCreateDeviceBitmap, we'll end up using off-screen memory to store
* the bits anyway (which would have been the main reason for implementing
* DrvSaveScreenBits).  So we may as well save some working set.
\**************************************************************************/

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV            },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV          },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV           },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface         },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface        },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode            },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer           },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape       },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette            },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits              },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt                },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut               },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes              },
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo                },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath            },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath              },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush          },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap    },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap    },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt            },
    {   INDEX_DrvDestroyFont,           (PFN) DrvDestroyFont           },
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo     },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw      },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw     },
    {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize           },
    {   INDEX_DrvTransparentBlt,        (PFN) DrvTransparentBlt        },
    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface         },
    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DrvIcmSetDeviceGammaRamp },
};

ULONG gcdrvfn = sizeof(gadrvfn) / sizeof(DRVFN);

/******************************Public*Routine******************************\
* BOOL DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.

    // Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = gcdrvfn;

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    return;
}

/******************************Public*Routine******************************\
* DHPDEV DrvEnablePDEV
*
* Initializes a bunch of fields for GDI, based on the mode we've been asked
* to do.  This is the first thing called after DrvEnableDriver, when GDI
* wants to get some information about us.
*
* (This function mostly returns back information; DrvEnableSurface is used
* for initializing the hardware and driver components.)
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
DEVMODEW*   pdm,            // Contains data pertaining to requested mode
PWSTR       pwszLogAddr,    // Logical address
ULONG       cPat,           // Count of standard patterns
HSURF*      phsurfPatterns, // Buffer for standard patterns
ULONG       cjCaps,         // Size of buffer for device caps 'pdevcaps'
ULONG*      pdevcaps,       // Buffer for device caps, also known as 'gdiinfo'
ULONG       cjDevInfo,      // Number of bytes in device info 'pdi'
DEVINFO*    pdi,            // Device information
HDEV        hdev,           // HDEV, used for callbacks
PWSTR       pwszDeviceName, // Device name
HANDLE      hDriver)        // Kernel driver handle
{
    PDEV*   ppdev;

    // Future versions of NT had better supply 'devcaps' and 'devinfo'
    // structures that are the same size or larger than the current
    // structures:

    if ((cjCaps < sizeof(GDIINFO)) || (cjDevInfo < sizeof(DEVINFO)))
    {
        DISPDBG((0, "DrvEnablePDEV - Buffer size too small"));
        goto ReturnFailure0;
    }

    // Allocate a physical device structure.  Note that we definitely
    // rely on the zero initialization:

    ppdev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
    if (ppdev == NULL)
    {
        DISPDBG((0, "DrvEnablePDEV - Failed EngAllocMem"));
        goto ReturnFailure0;
    }

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and
    // devinfo:

    if (!bInitializeModeFields(ppdev, (GDIINFO*) pdevcaps, pdi, pdm))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializeModeFields"));
        goto ReturnFailure1;
    }

    // Initialize palette information.

    if (!bInitializePalette(ppdev, pdi))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializePalette"));
        goto ReturnFailure1;
    }

    return((DHPDEV) ppdev);

ReturnFailure1:
    DrvDisablePDEV((DHPDEV) ppdev);

ReturnFailure0:
    DISPDBG((0, "Failed DrvEnablePDEV"));

    return(0);
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
* Note that this function will be called when previewing modes in the
* Display Applet, but not at system shutdown.  If you need to reset the
* hardware at shutdown, you can do it in the miniport by providing a
* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
*
* Note: In an error, we may call this before DrvEnablePDEV is done.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV  dhpdev)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    vUninitializePalette(ppdev);
    EngFreeMem(ppdev);
}

/******************************Public*Routine******************************\
* VOID DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    ((PDEV*) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* HSURF DrvEnableSurface
*
* Creates the drawing surface, initializes the hardware, and initializes
* driver components.  This function is called after DrvEnablePDEV, and
* performs the final device initialization.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PDEV*   ppdev;
    HSURF   hsurf;
    SIZEL   sizl;
    DSURF*  pdsurf;
    VOID*   pvTmpBuffer;
    BYTE*   pjScreen;
    LONG    lDelta;
    FLONG   flHooks;

    ppdev = (PDEV*) dhpdev;

    /////////////////////////////////////////////////////////////////////
    // First enable all the subcomponents.
    //
    // Note that the order in which these 'Enable' functions are called
    // may be significant in low off-screen memory conditions, because
    // the off-screen heap manager may fail some of the later
    // allocations...

    if (!bEnableHardware(ppdev))
        goto ReturnFailure;

    if (!bEnableBanking(ppdev))
        goto ReturnFailure;

    if (!bEnableOffscreenHeap(ppdev))
        goto ReturnFailure;

    if (!bEnablePointer(ppdev))
        goto ReturnFailure;

    if (!bEnableText(ppdev))
        goto ReturnFailure;

    if (!bEnableBrushCache(ppdev))
        goto ReturnFailure;

    if (!bEnablePalette(ppdev))
        goto ReturnFailure;

    if (!bEnableDirectDraw(ppdev))
        goto ReturnFailure;

    /////////////////////////////////////////////////////////////////////
    // Now create our private surface structure.
    //
    // Whenever we get a call to draw directly to the screen, we'll get
    // passed a pointer to a SURFOBJ whose 'dhpdev' field will point
    // to our PDEV structure, and whose 'dhsurf' field will point to the
    // following DSURF structure.
    //
    // Every device bitmap we create in DrvCreateDeviceBitmap will also
    // have its own unique DSURF structure allocated (but will share the
    // same PDEV).  To make our code more polymorphic for handling drawing
    // to either the screen or an off-screen bitmap, we have the same
    // structure for both.

    pdsurf = &ppdev->dsurfScreen;

    pdsurf->dt       = 0; 
    pdsurf->x        = 0;
    pdsurf->y        = 0;
    pdsurf->fpVidMem = 0;
    pdsurf->ppdev    = ppdev;

    /////////////////////////////////////////////////////////////////////
    // Next, have GDI create the actual surface SURFOBJ structure.

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    // Create the primary surface.  This defaults to a 'device-managed'
    // surface, but EngModifySurface can change that.

    hsurf = EngCreateDeviceSurface((DHSURF) pdsurf, 
                                   sizl, 
                                   ppdev->iBitmapFormat);
    if (hsurf == 0)
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngCreateDeviceSurface"));
        goto ReturnFailure;
    }

    if ((ppdev->flCaps & CAPS_NEW_MMIO) &&
        !(ppdev->flCaps & CAPS_NO_DIRECT_ACCESS))
    {
        // On all cards where we linearly map the frame buffer, create our 
        // drawing surface as a GDI-managed surface, meaning that we give
        // GDI a pointer to the framebuffer and GDI can draw on the bits
        // directly.  This will allow us good performance with drawing such 
        // as GradientFills, even though our hardware can't accelerate the 
        // drawing and so we don't hook DrvGradientFill.  This way GDI can 
        // do write-combined writes directly to the framebuffer and still be 
        // very fast.
        //
        // Note that this requires that we hook DrvSynchronize and
        // set HOOK_SYNCHRONIZE.

        pjScreen = ppdev->pjScreen;
        lDelta   = ppdev->lDelta;
        flHooks  = ppdev->flHooks | HOOK_SYNCHRONIZE;
    }
    else
    {
        // Ugh, we're running on an ancient S3 card where we can't completely
        // map the entire frame buffer into memory.  We have to create the
        // primary surface as a 'GDI-opaque' device-managed surface, and GDI
        // will be forced to go through only Drv calls that we've hooked.
        // (In this case, drawing such as GradientFills will be pathetically
        // slow.)

        pjScreen = NULL;
        lDelta   = 0;
        flHooks  = ppdev->flHooks;
    }

    // Note that this call is new to NT5, and takes the place of
    // EngAssociateSurface. 

    if (!EngModifySurface(hsurf, 
                          ppdev->hdevEng, 
                          flHooks,
                          MS_NOTSYSTEMMEMORY,    // It's in video memory
                          (DHSURF) pdsurf,
                          pjScreen,
                          lDelta,
                          NULL))
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngModifySurface"));
        goto ReturnFailure;
    }

    ppdev->hsurfScreen = hsurf;             // Remember it for clean-up
    ppdev->bEnabled = TRUE;                 // We'll soon be in graphics mode

    // Create our generic temporary buffer, which may be used by any
    // component.

    pvTmpBuffer = EngAllocMem(0, TMP_BUFFER_SIZE, ALLOC_TAG);
    if (pvTmpBuffer == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed VirtualAlloc"));
        goto ReturnFailure;
    }

    ppdev->pvTmpBuffer = pvTmpBuffer;

    DISPDBG((5, "Passed DrvEnableSurface"));

    return(hsurf);

ReturnFailure:
    DrvDisableSurface((DHPDEV) ppdev);

    DISPDBG((0, "Failed DrvEnableSurface"));

    return(0);
}

/******************************Public*Routine******************************\
* VOID DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
* Note that this function will be called when previewing modes in the
* Display Applet, but not at system shutdown.  If you need to reset the
* hardware at shutdown, you can do it in the miniport by providing a
* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
*
* Note: In an error case, we may call this before DrvEnableSurface is
*       completely done.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    // Note: In an error case, some of the following relies on the
    //       fact that the PDEV is zero-initialized, so fields like
    //       'hsurfScreen' will be zero unless the surface has been
    //       sucessfully initialized, and makes the assumption that
    //       EngDeleteSurface can take '0' as a parameter.

    vDisableDirectDraw(ppdev);
    vDisablePalette(ppdev);
    vDisableBrushCache(ppdev);
    vDisableText(ppdev);
    vDisablePointer(ppdev);
    vDisableOffscreenHeap(ppdev);
    vDisableBanking(ppdev);
    vDisableHardware(ppdev);

    EngFreeMem(ppdev->pvTmpBuffer);
    EngDeleteSurface(ppdev->hsurfScreen);
}

/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo
*
* Will be called after DrvEnablesurface.  Will be called twice before 
* DrvEnableDirectDraw is called.
*
\**************************************************************************/

BOOL DrvGetDirectDrawInfo(
DHPDEV          dhpdev,
DD_HALINFO*     pHalInfo,
DWORD*          pdwNumHeaps,
VIDEOMEMORY*    pvmList,            // Will be NULL on first call
DWORD*          pdwNumFourCC,
DWORD*          pdwFourCC)          // Will be NULL on first call
{
    PDEV*       ppdev;
    LONGLONG    li;
    DWORD       cProcessors;
    DWORD       cHeaps;

    ppdev = (PDEV*) dhpdev;

    *pdwNumFourCC = 0;
    *pdwNumHeaps = 0;

    // We may not support DirectDraw on this card.
    //
    // The 765 (Trio64V+) has a bug such that writing to the frame
    // buffer during an accelerator operation may cause a hang if
    // you do the write soon enough after starting the blt.  (There is
    // a small window of opportunity.)  On UP machines, the context
    // switch time seems to be enough to avoid the problem.  However,
    // on MP machines, we'll have to disable direct draw.
    //
    // NOTE: We can identify the 765 since it is the only chip with
    //       the CAPS_STREAMS_CAPABLE flag.

    if (ppdev->flCaps & CAPS_STREAMS_CAPABLE) 
    {
        if (!EngQuerySystemAttribute(EngNumberOfProcessors, &cProcessors) ||
            (cProcessors != 1))
        {
            return(FALSE);
        }
    }

    if (!(ppdev->flCaps & CAPS_NEW_MMIO) ||
        (ppdev->flCaps & CAPS_NO_DIRECT_ACCESS))
    {
        return(FALSE);
    }

    pHalInfo->dwSize = sizeof(*pHalInfo);

    // Current primary surface attributes.  Since HalInfo is zero-initialized
    // by GDI, we only have to fill in the fields which should be non-zero:

    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lDelta;

    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = 8 * ppdev->cjPelSize;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

    // The S3 has to do everything using 'rectangular' memory, because
    // the accelerator doesn't know how to set arbitrary strides.

    cHeaps = 0;

    // Snag a pointer to the video-memory list so that we can use it to
    // call back to DirectDraw to allocate video memory:

    ppdev->pvmList = pvmList;

    // Create one heap to describe the unused portion of video
    // memory to the right of the visible screen (if any):

    if (ppdev->cxScreen < ppdev->cxHeap)
    {
        cHeaps++;

        if (pvmList != NULL)
        {
            pvmList->dwFlags        = VIDMEM_ISRECTANGULAR;
            pvmList->fpStart        = ppdev->cxScreen * ppdev->cjPelSize;
            pvmList->dwWidth        = (ppdev->cxHeap - ppdev->cxScreen) 
                                    * ppdev->cjPelSize;
            pvmList->dwHeight       = ppdev->cyScreen;
            pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            pvmList++;
        }
    }

    // Create another heap to describe the unused portion of video
    // memory below the visible screen (if any):

    if (ppdev->cyScreen < ppdev->cyHeap)
    {
        cHeaps++;

        if (pvmList != NULL)
        {
            pvmList->dwFlags        = VIDMEM_ISRECTANGULAR;
            pvmList->fpStart        = ppdev->cyScreen * ppdev->lDelta;
            pvmList->dwWidth        = ppdev->cxHeap * ppdev->cjPelSize;
            pvmList->dwHeight       = ppdev->cyHeap - ppdev->cyScreen;
            pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            pvmList++;
        }
    }

    // Update the number of heaps:

    ppdev->cHeaps = cHeaps;
    *pdwNumHeaps  = cHeaps;

    // dword alignment must be guaranteed for off-screen surfaces:

    pHalInfo->vmiData.dwOffscreenAlign = 4;

    // Capabilities supported:

    pHalInfo->ddCaps.dwCaps = DDCAPS_BLT
                            | DDCAPS_BLTCOLORFILL
                            | DDCAPS_COLORKEY;

    pHalInfo->ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT;

    pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN
                                    | DDSCAPS_PRIMARYSURFACE
                                    | DDSCAPS_FLIP;

    // The Trio 64V+ has overlay streams capabilities which are a superset
    // of the above:

    if (ppdev->flCaps & CAPS_STREAMS_CAPABLE)
    {
        // Overlays need 8-byte alignment.  Note that if 24bpp overlays are
        // ever supported, this will have to change to compensate:

        pHalInfo->vmiData.dwOverlayAlign = 8;

        pHalInfo->ddCaps.dwCaps |= DDCAPS_OVERLAY
                                 | DDCAPS_OVERLAYSTRETCH
                                 | DDCAPS_OVERLAYFOURCC
                                 | DDCAPS_OVERLAYCANTCLIP;

        pHalInfo->ddCaps.dwFXCaps |= DDFXCAPS_OVERLAYSTRETCHX
                                   | DDFXCAPS_OVERLAYSTRETCHY;

        // We support only destination colour keying because that's the
        // only permutation we've had a chance to test.

        pHalInfo->ddCaps.dwCKeyCaps |= DDCKEYCAPS_DESTOVERLAY;

        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_OVERLAY;

        *pdwNumFourCC = 1;
        if (pdwFourCC)
        {
            pdwFourCC[0] = FOURCC_YUY2;
        }

        pHalInfo->ddCaps.dwMaxVisibleOverlays = 1;

        pHalInfo->ddCaps.dwMinOverlayStretch   = ppdev->ulMinOverlayStretch;
        pHalInfo->ddCaps.dwMinLiveVideoStretch = ppdev->ulMinOverlayStretch;
        pHalInfo->ddCaps.dwMinHwCodecStretch   = ppdev->ulMinOverlayStretch;

        pHalInfo->ddCaps.dwMaxOverlayStretch   = 9999;
        pHalInfo->ddCaps.dwMaxLiveVideoStretch = 9999;
        pHalInfo->ddCaps.dwMaxHwCodecStretch   = 9999;
    }

    // The 868 and 968 have a pixel formatter which is capable of doing
    // colour space conversions and hardware stretching from off-screen
    // surfaces:

    else if (ppdev->flCaps & CAPS_PIXEL_FORMATTER)
    {
        pHalInfo->ddCaps.dwCaps |= DDCAPS_BLTSTRETCH;

        pHalInfo->ddCaps.dwFXCaps |= DDFXCAPS_BLTSTRETCHX
                                   | DDFXCAPS_BLTSTRETCHY;

        // YUV is supported only above 8bpp:

        if (ppdev->iBitmapFormat != BMF_8BPP)
        {
            pHalInfo->ddCaps.dwCaps |= DDCAPS_BLTFOURCC;

            *pdwNumFourCC = 1;
            if (pdwFourCC)
            {
                *pdwFourCC = FOURCC_YUY2;
            }
        }
    }

    // Tell DirectDraw that we support additional callbacks via 
    // DdGetDriverInfo:

    pHalInfo->GetDriverInfo = DdGetDriverInfo;
    pHalInfo->dwFlags |= DDHALINFO_GETDRIVERINFOSET;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DrvEnableDirectDraw
*
* This function is called by GDI when a new mode is set, immediately after
* it calls our DrvEnableSurface and DrvGetDirectDrawInfo.
*
\**************************************************************************/

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS*           pCallBacks,
DD_SURFACECALLBACKS*    pSurfaceCallBacks,
DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank;
    pCallBacks->MapMemory            = DdMapMemory;
    pCallBacks->dwFlags              = DDHAL_CB32_WAITFORVERTICALBLANK
                                     | DDHAL_CB32_MAPMEMORY;

    pSurfaceCallBacks->Blt           = DdBlt;
    pSurfaceCallBacks->Flip          = DdFlip;
    pSurfaceCallBacks->Lock          = DdLock;
    pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus;
    pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus;
    pSurfaceCallBacks->dwFlags       = DDHAL_SURFCB32_BLT
                                     | DDHAL_SURFCB32_FLIP
                                     | DDHAL_SURFCB32_LOCK
                                     | DDHAL_SURFCB32_GETBLTSTATUS
                                     | DDHAL_SURFCB32_GETFLIPSTATUS;

    // We can do overlays only when the Streams processor is enabled:

    if (ppdev->flCaps & CAPS_STREAMS_CAPABLE)
    {
        pCallBacks->CreateSurface             = DdCreateSurface;
        pCallBacks->CanCreateSurface          = DdCanCreateSurface;
        pCallBacks->dwFlags                  |= DDHAL_CB32_CREATESURFACE
                                              | DDHAL_CB32_CANCREATESURFACE;

        pSurfaceCallBacks->SetColorKey        = DdSetColorKey;
        pSurfaceCallBacks->UpdateOverlay      = DdUpdateOverlay;
        pSurfaceCallBacks->SetOverlayPosition = DdSetOverlayPosition;
        pSurfaceCallBacks->dwFlags           |= DDHAL_SURFCB32_SETCOLORKEY
                                              | DDHAL_SURFCB32_UPDATEOVERLAY
                                              | DDHAL_SURFCB32_SETOVERLAYPOSITION;
        ppdev->ulColorKey                     = 0;
    }

    // We can do blts with funky surface formats only when the pixel
    // formatter is enabled:

    else if (ppdev->flCaps & CAPS_PIXEL_FORMATTER)
    {
        pCallBacks->CreateSurface     = DdCreateSurface;
        pCallBacks->CanCreateSurface  = DdCanCreateSurface;
        pCallBacks->dwFlags          |= DDHAL_CB32_CREATESURFACE
                                      | DDHAL_CB32_CANCREATESURFACE;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDirectDraw
*
* This function is called by GDI when the driver is to be disabled, just
* before it calls DrvDisableSurface. 
*
\**************************************************************************/

VOID DrvDisableDirectDraw(
DHPDEV      dhpdev)
{
}

/******************************Public*Routine******************************\
* VOID DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

BOOL DrvAssertMode(
DHPDEV  dhpdev,
BOOL    bEnable)
{
    PDEV* ppdev;

    ppdev = (PDEV*) dhpdev;

    if (!bEnable)
    {
        //////////////////////////////////////////////////////////////
        // Disable - Switch to full-screen mode

        vAssertModeDirectDraw(ppdev, FALSE);

        vAssertModePalette(ppdev, FALSE);

        vAssertModeBrushCache(ppdev, FALSE);

        vAssertModeText(ppdev, FALSE);

        vAssertModePointer(ppdev, FALSE);

        if (bAssertModeOffscreenHeap(ppdev, FALSE))
        {
            vAssertModeBanking(ppdev, FALSE);

            if (bAssertModeHardware(ppdev, FALSE))
            {
                ppdev->bEnabled = FALSE;

                return(TRUE);
            }

            //////////////////////////////////////////////////////////
            // We failed to switch to full-screen.  So undo everything:

            vAssertModeBanking(ppdev, TRUE);

            bAssertModeOffscreenHeap(ppdev, TRUE);  // We don't need to check
        }                                           //   return code with TRUE

        // there is HW setup in bEnablePointer that needs to be done at assert time too
        // coming back from full-screen DOS or hibernate so call enablepointer which
        // then calls vAssertModePointer itself.  In 8bpp, the DAC resolution was not
        // being set correctly after FSdos or Hib. causing screen to be dim

        bEnablePointer(ppdev);

        vAssertModeText(ppdev, TRUE);

        vAssertModeBrushCache(ppdev, TRUE);

        vAssertModePalette(ppdev, TRUE);

        vAssertModeDirectDraw(ppdev, TRUE);
    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:

        if (bAssertModeHardware(ppdev, TRUE))
        {
            vAssertModeBanking(ppdev, TRUE);

            bAssertModeOffscreenHeap(ppdev, TRUE);  // We don't need to check
                                                    //   return code with TRUE
            bEnablePointer(ppdev);

            vAssertModeText(ppdev, TRUE);

            vAssertModeBrushCache(ppdev, TRUE);

            vAssertModePalette(ppdev, TRUE);

            vAssertModeDirectDraw(ppdev, TRUE);

            ppdev->bEnabled = TRUE;

            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* ULONG DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE      hDriver,
ULONG       cjSize,
DEVMODEW*   pdm)
{
    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    cModes = getAvailableModes(hDriver,
                            (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                            &cbModeSize);
    if (cModes == 0)
    {
        DISPDBG((0, "DrvGetModes failed to get mode information"));
        return(0);
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the
        // output buffer
        //

        cbOutputSize = 0;

        pVideoTemp = pVideoModeInformation;

        do
        {
            if (pVideoTemp->Length != 0)
            {
                if (cOutputModes == 0)
                {
                    break;
                }

                //
                // Zero the entire structure to start off with.
                //

                memset(pdm, 0, sizeof(DEVMODEW));

                //
                // Set the name of the device to the name of the DLL.
                //

                memcpy(pdm->dmDeviceName, DLL_NAME, sizeof(DLL_NAME));

                pdm->dmSpecVersion      = DM_SPECVERSION;
                pdm->dmDriverVersion    = DM_SPECVERSION;
                pdm->dmSize             = sizeof(DEVMODEW);
                pdm->dmDriverExtra      = DRIVER_EXTRA_SIZE;

                pdm->dmBitsPerPel       = pVideoTemp->NumberOfPlanes *
                                          pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth        = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight       = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;
                pdm->dmDisplayFlags     = 0;

                pdm->dmFields           = DM_BITSPERPEL       |
                                          DM_PELSWIDTH        |
                                          DM_PELSHEIGHT       |
                                          DM_DISPLAYFREQUENCY |
                                          DM_DISPLAYFLAGS     ;

                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((ULONG_PTR)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);


        } while (--cModes);
    }

    EngFreeMem(pVideoModeInformation);

    return(cbOutputSize);
}

/******************************Public*Routine******************************\
* BOOL bAssertModeHardware
*
* Sets the appropriate hardware state for graphics mode or full-screen.
*
\**************************************************************************/

BOOL bAssertModeHardware(
PDEV* ppdev,
BOOL  bEnable)
{
    BYTE*                   pjIoBase;
    BYTE*                   pjMmBase;
    DWORD                   ReturnedDataLength;
    ULONG                   ulReturn;
    BYTE                    jExtendedMemoryControl;
    VIDEO_MODE_INFORMATION  VideoModeInfo;
    LONG                    cjEndOfFrameBuffer;
    LONG                    cjPointerOffset;
    LONG                    lDelta;
    ULONG                   ulMiscState;

    pjIoBase = ppdev->pjIoBase;
    pjMmBase = ppdev->pjMmBase;

    if (bEnable)
    {
        // Call the miniport via an IOCTL to set the graphics mode.

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SET_CURRENT_MODE,
                               &ppdev->ulMode,  // input buffer
                               sizeof(DWORD),
                               NULL,
                               0,
                               &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - Failed VIDEO_SET_CURRENT_MODE"));
            goto ReturnFalse;
        }

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_QUERY_CURRENT_MODE,
                               NULL,
                               0,
                               &VideoModeInfo,
                               sizeof(VideoModeInfo),
                               &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - failed VIDEO_QUERY_CURRENT_MODE"));
            goto ReturnFalse;
        }

        #if DEBUG_HEAP
            VideoModeInfo.VideoMemoryBitmapWidth  = VideoModeInfo.VisScreenWidth;
            VideoModeInfo.VideoMemoryBitmapHeight = VideoModeInfo.VisScreenHeight;
        #endif

        // The following variables are determined only after the initial
        // modeset:

        ppdev->lDelta   = VideoModeInfo.ScreenStride;
        ppdev->flCaps   = VideoModeInfo.DriverSpecificAttributeFlags;
        ppdev->cxMemory = VideoModeInfo.VideoMemoryBitmapWidth;
        ppdev->cxHeap   = VideoModeInfo.VideoMemoryBitmapWidth;
        ppdev->cyMemory = VideoModeInfo.VideoMemoryBitmapHeight;
        ppdev->cyHeap   = VideoModeInfo.VideoMemoryBitmapHeight;

        ppdev->bMmIo = ((ppdev->flCaps & CAPS_MM_IO) > 0);

        // If we're using the S3 hardware pointer, reserve the last 1k of
        // the frame buffer to store the pointer shape:

        if (!(ppdev->flCaps & (CAPS_SW_POINTER | CAPS_DAC_POINTER)))
        {
            // Byte offset from start of frame buffer to end:

            cjEndOfFrameBuffer = ppdev->cyMemory * ppdev->lDelta;

            // We'll reserve the end of off-screen memory for the hardware
            // pointer shape.  Unfortunately, the S3 chips have a bug
            // where the shape has to be stored on a 1K multiple,
            // regardless of what the current screen stride is.

            cjPointerOffset = (cjEndOfFrameBuffer - HW_POINTER_TOTAL_SIZE)
                            & ~(HW_POINTER_TOTAL_SIZE - 1);

            // Figure out the coordinate where the pointer shape starts:

            lDelta = ppdev->lDelta;

            ppdev->cjPointerOffset = cjPointerOffset;
            ppdev->yPointerShape   = (cjPointerOffset / lDelta);
            ppdev->xPointerShape   =
              CONVERT_FROM_BYTES((cjPointerOffset % lDelta), ppdev);

            if (ppdev->yPointerShape >= ppdev->cyScreen)
            {
                // There's enough room for the pointer shape at the
                // bottom of off-screen memory; reserve its room by
                // lying about how much off-screen memory there is:

                ppdev->cyMemory = ppdev->yPointerShape;
            }
            else
            {
                // There's not enough room for the pointer shape in
                // off-screen memory; we'll have to simulate:

                ppdev->flCaps |= CAPS_SW_POINTER;
            }
        }

        // Do some parameter checking on the values that the miniport
        // returned to us:

        ASSERTDD(ppdev->cxMemory >= ppdev->cxScreen, "Invalid cxMemory");
        ASSERTDD(ppdev->cyMemory >= ppdev->cyScreen, "Invalid cyMemory");
        ASSERTDD((ppdev->flCaps &
                 (CAPS_NEW_BANK_CONTROL | CAPS_NEWER_BANK_CONTROL)) ||
                 ((ppdev->cxMemory <= 1024) && (ppdev->cyMemory <= 1024)),
                 "Have to have new bank control if more than 1meg memory");
        ASSERTDD((ppdev->flCaps & (CAPS_SW_POINTER | CAPS_DAC_POINTER)) !=
                 (CAPS_SW_POINTER | CAPS_DAC_POINTER),
                 "Should not set both Software and DAC cursor flags");
        ASSERTDD(!(ppdev->flCaps & CAPS_MM_IO) ||
                 (ppdev->flCaps & (CAPS_MM_TRANSFER | CAPS_MM_32BIT_TRANSFER)),
                 "Must enable memory-mapped transfer if memory-mapped I/O");

        // First thing we do is unlock the accelerator registers:

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        OUTPW(pjIoBase, CRTC_INDEX, ((SYSCTL_UNLOCK << 8) | CR39));
        OUTPW(pjIoBase, CRTC_INDEX, ((REG_UNLOCK_1 << 8) | S3R8));

        // Enable memory-mapped IO.  Note that ulMiscState should not be
        // read on non-memory mapped I/O S3's because it does not exist
        // on 911/924's.

        if (ppdev->flCaps & CAPS_MM_IO)
        {
            OUTP(pjIoBase, CRTC_INDEX, 0x53);

            jExtendedMemoryControl = INP(pjIoBase, CRTC_DATA);

            OUTP(pjIoBase, CRTC_DATA, jExtendedMemoryControl | 0x10);

            // Read the default MULTI_MISC register state.

            IO_GP_WAIT(ppdev);                  // Wait so we don't interfere with any
                                                //   pending commands waiting on the
                                                //   FIFO
            IO_READ_SEL(ppdev, 6);              // We'll be reading index 0xE
            IO_GP_WAIT(ppdev);                  // Wait until that's processed
            IO_RD_REG_DT(ppdev, ulMiscState);   // Read ulMiscState

            // Make the colour and mask registers '32-bit'.
            //
            // NOTE: This is what precludes enabling MM I/O on 928 boards.

            ulMiscState |= 0x0200;
            IO_MULT_MISC(ppdev, ulMiscState);

            ppdev->ulMiscState = ulMiscState;
        }

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);

        // Then set the rest of the default registers:

        vResetClipping(ppdev);

        if (ppdev->flCaps & CAPS_MM_IO)
        {
            IO_FIFO_WAIT(ppdev, 1);
            MM_WRT_MASK(ppdev, pjMmBase, -1);
        }
        else
        {
            if (DEPTH32(ppdev))
            {
                IO_FIFO_WAIT(ppdev, 2);
                IO_WRT_MASK32(ppdev, -1);
            }
            else
            {
                IO_FIFO_WAIT(ppdev, 1);
                IO_WRT_MASK(ppdev, -1);
            }
        }
    }
    else
    {
        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there:

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_RESET_DEVICE,
                               NULL,
                               0,
                               NULL,
                               0,
                               &ulReturn))
        {
            DISPDBG((0, "bAssertModeHardware - Failed reset IOCTL"));
            goto ReturnFalse;
        }
    }

    DISPDBG((5, "Passed bAssertModeHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bAssertModeHardware"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bEnableHardware
*
* Puts the hardware in the requested mode and initializes it.
*
* Note: Should be called before any access is done to the hardware from
*       the display driver.
*
\**************************************************************************/

BOOL bEnableHardware(
PDEV*   ppdev)
{
    BYTE*                       pjIoBase;
    VIDEO_PUBLIC_ACCESS_RANGES  VideoAccessRange[2];
    VIDEO_MEMORY                VideoMemory;
    VIDEO_MEMORY_INFORMATION    VideoMemoryInfo;
    DWORD                       ReturnedDataLength;
    UCHAR*                      pj;
    USHORT*                     pw;
    ULONG*                      pd;
    ULONG                       i;

    // We need a critical section merely because of some S3 weirdness:
    // both the bank control registers and the cursor registers have
    // to be accessed through the shared CRTC registers.  We want to
    // set the GCAPS_ASYNCMOVE flag to allow the cursor to move even
    // while we're using the bank registers for a blt -- so we have to
    // synchronize all accesses to the CRTC registers.
    //
    // (Note that in the case of GCAPS_ASYNCMOVE, GDI automatically
    // synchronizes with DrvSetPalette, so you don't have to worry
    // about overlap between asynchronous cursor moves and the palette
    // registers.)

    ppdev->csCrtc = EngCreateSemaphore();
    if (ppdev->csCrtc == 0)
    {
        DISPDBG((0, "bEnableHardware - Error creating CRTC semaphore"));
        goto ReturnFalse;
    }

    // Map io ports into virtual memory:

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                           NULL,                      // input buffer
                           0,
                           &VideoAccessRange,         // output buffer
                           sizeof(VideoAccessRange),
                           &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware - Initialization error mapping IO port base"));
        goto ReturnFalse;
    }

    ppdev->pjIoBase = (UCHAR*) VideoAccessRange[0].VirtualAddress;
    ppdev->pjMmBase = (BYTE*)  VideoAccessRange[1].VirtualAddress;

    pjIoBase = ppdev->pjIoBase;

    // Get the linear memory address range.

    VideoMemory.RequestedVirtualAddress = NULL;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                           &VideoMemory,      // input buffer
                           sizeof(VIDEO_MEMORY),
                           &VideoMemoryInfo,  // output buffer
                           sizeof(VideoMemoryInfo),
                           &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware - Error mapping buffer address"));
        goto ReturnFalse;
    }

    // Record the Frame Buffer Linear Address.

    ppdev->pjScreen = (BYTE*) VideoMemoryInfo.FrameBufferBase;
    ppdev->cjBank   =         VideoMemoryInfo.FrameBufferLength;

    DISPDBG((1, "pjScreen: %lx  pjMmBase: %lx", ppdev->pjScreen, ppdev->pjMmBase));

    // Set all the register addresses.

    ppdev->ioCur_y          = pjIoBase + CUR_Y;
    ppdev->ioCur_x          = pjIoBase + CUR_X;
    ppdev->ioDesty_axstp    = pjIoBase + DEST_Y;
    ppdev->ioDestx_diastp   = pjIoBase + DEST_X;
    ppdev->ioErr_term       = pjIoBase + ERR_TERM;
    ppdev->ioMaj_axis_pcnt  = pjIoBase + MAJ_AXIS_PCNT;
    ppdev->ioGp_stat_cmd    = pjIoBase + CMD;
    ppdev->ioShort_stroke   = pjIoBase + SHORT_STROKE;
    ppdev->ioBkgd_color     = pjIoBase + BKGD_COLOR;
    ppdev->ioFrgd_color     = pjIoBase + FRGD_COLOR;
    ppdev->ioWrt_mask       = pjIoBase + WRT_MASK;
    ppdev->ioRd_mask        = pjIoBase + RD_MASK;
    ppdev->ioColor_cmp      = pjIoBase + COLOR_CMP;
    ppdev->ioBkgd_mix       = pjIoBase + BKGD_MIX;
    ppdev->ioFrgd_mix       = pjIoBase + FRGD_MIX;
    ppdev->ioMulti_function = pjIoBase + MULTIFUNC_CNTL;
    ppdev->ioPix_trans      = pjIoBase + PIX_TRANS;

    for (pw = (USHORT*) ppdev->pjMmBase, i = 0; i < XFER_BUFFERS; i++, pw += 2)
    {
        ppdev->apwMmXfer[i] = pw;
    }
    for (pd = (ULONG*) ppdev->pjMmBase, i = 0; i < XFER_BUFFERS; i++, pd++)
    {
        ppdev->apdMmXfer[i] = pd;
    }

    // Now we can set the mode, unlock the accelerator, and reset the
    // clipping:

    if (!bAssertModeHardware(ppdev, TRUE))
        goto ReturnFalse;

    if (ppdev->flCaps & CAPS_MM_IO)
    {
        // Can do memory-mapped IO:

        ppdev->pfnFillSolid         = vMmFillSolid;
        ppdev->pfnFillPat           = vMmFillPatFast;
        ppdev->pfnXfer1bpp          = vMmXfer1bpp;
        ppdev->pfnXfer4bpp          = vMmXfer4bpp;
        ppdev->pfnXferNative        = vMmXferNative;
        ppdev->pfnCopyBlt           = vMmCopyBlt;
        ppdev->pfnFastPatRealize    = vMmFastPatRealize;
        ppdev->pfnTextOut           = bMmTextOut;
        ppdev->pfnLineToTrivial     = vMmLineToTrivial;
        ppdev->pfnLineToClipped     = vMmLineToClipped;
        ppdev->pfnCopyTransparent   = vMmCopyTransparent;

        if (ppdev->flCaps & CAPS_MM_32BIT_TRANSFER)
            ppdev->pfnImageTransfer = vMmImageTransferMm32;
        else
            ppdev->pfnImageTransfer = vMmImageTransferMm16;

        // On some cards, it may be faster to use the old I/O based
        // glyph routine, which uses the CPU to draw all the glyphs
        // to a monochrome buffer, and then uses the video hardware
        // to colour expand the result:

        if (!(ppdev->flCaps & CAPS_MM_GLYPH_EXPAND))
            ppdev->pfnTextOut = bIoTextOut;

        if (ppdev->flCaps & CAPS_NEW_MMIO)
        {
            ppdev->pfnTextOut       = bNwTextOut;
            ppdev->pfnLineToTrivial = vNwLineToTrivial;
            ppdev->pfnLineToClipped = vNwLineToClipped;
        }
    }
    else
    {
        // Have to do IN/OUTs:

        ppdev->pfnFillSolid         = vIoFillSolid;
        ppdev->pfnFillPat           = vIoFillPatFast;
                            // bEnableBrushCache may override this value

        ppdev->pfnXfer1bpp          = vIoXfer1bpp;
        ppdev->pfnXfer4bpp          = vIoXfer4bpp;
        ppdev->pfnXferNative        = vIoXferNative;
        ppdev->pfnCopyBlt           = vIoCopyBlt;
        ppdev->pfnFastPatRealize    = vIoFastPatRealize;
        ppdev->pfnTextOut           = bIoTextOut;
        ppdev->pfnLineToTrivial     = vIoLineToTrivial;
        ppdev->pfnLineToClipped     = vIoLineToClipped;
        ppdev->pfnCopyTransparent   = vIoCopyTransparent;

        if (ppdev->flCaps & CAPS_MM_TRANSFER)
            ppdev->pfnImageTransfer = vIoImageTransferMm16;
        else
            ppdev->pfnImageTransfer = vIoImageTransferIo16;
    }

    #if DBG
    {
        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        OUTP(pjIoBase, CRTC_INDEX, 0x30);

        DISPDBG((0, "Chip: %lx Bank: %lx Width: %li Height: %li Stride: %li Flags: %08lx",
                (ULONG) INP(pjIoBase, CRTC_DATA), ppdev->cjBank, ppdev->cxMemory, ppdev->cyMemory,
                ppdev->lDelta, ppdev->flCaps));

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);
    }
    #endif

    DISPDBG((5, "Passed bEnableHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bEnableHardware"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vDisableHardware
*
* Undoes anything done in bEnableHardware.
*
* Note: In an error case, we may call this before bEnableHardware is
*       completely done.
*
\**************************************************************************/

VOID vDisableHardware(
PDEV*   ppdev)
{
    DWORD        ReturnedDataLength;
    VIDEO_MEMORY VideoMemory[2];

    VideoMemory[0].RequestedVirtualAddress = ppdev->pjScreen;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                           VideoMemory,
                           sizeof(VIDEO_MEMORY),
                           NULL,
                           0,
                           &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_UNMAP_VIDEO"));
    }

    VideoMemory[0].RequestedVirtualAddress = ppdev->pjIoBase;
    VideoMemory[1].RequestedVirtualAddress = ppdev->pjMmBase;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                           VideoMemory,
                           sizeof(VideoMemory),
                           NULL,
                           0,
                           &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_FREE_PUBLIC_ACCESS"));
    }

    EngDeleteSemaphore(ppdev->csCrtc);
}

/******************************Public*Routine******************************\
* BOOL bInitializeModeFields
*
* Initializes a bunch of fields in the pdev, devcaps (aka gdiinfo), and
* devinfo based on the requested mode.
*
\**************************************************************************/

BOOL bInitializeModeFields(
PDEV*     ppdev,
GDIINFO*  pgdi,
DEVINFO*  pdi,
DEVMODEW* pdm)
{
    ULONG                   cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer;
    PVIDEO_MODE_INFORMATION pVideoModeSelected;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    BOOL                    bSelectDefault;
    VIDEO_MODE_INFORMATION  VideoModeInformation;
    ULONG                   cbModeSize;

    // Call the miniport to get mode information

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);
    if (cModes == 0)
        goto ReturnFalse;

    // Now see if the requested mode has a match in that table.

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    if ((pdm->dmPelsWidth        == 0) &&
        (pdm->dmPelsHeight       == 0) &&
        (pdm->dmBitsPerPel       == 0) &&
        (pdm->dmDisplayFrequency == 0))
    {
        DISPDBG((1, "Default mode requested"));
        bSelectDefault = TRUE;
    }
    else
    {
        DISPDBG((1, "Requested mode..."));
        DISPDBG((1, "   Screen width  -- %li", pdm->dmPelsWidth));
        DISPDBG((1, "   Screen height -- %li", pdm->dmPelsHeight));
        DISPDBG((1, "   Bits per pel  -- %li", pdm->dmBitsPerPel));
        DISPDBG((1, "   Frequency     -- %li", pdm->dmDisplayFrequency));

        bSelectDefault = FALSE;
    }

    while (cModes--)
    {
        if (pVideoTemp->Length != 0)
        {
            DISPDBG((8, "   Checking against miniport mode:"));
            DISPDBG((8, "      Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((8, "      Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((8, "      Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                      pVideoTemp->NumberOfPlanes));
            DISPDBG((8, "      Frequency     -- %li", pVideoTemp->Frequency));

            if (bSelectDefault ||
                ((pVideoTemp->VisScreenWidth  == pdm->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pdm->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                  pVideoTemp->NumberOfPlanes  == pdm->dmBitsPerPel) &&
                 (pVideoTemp->Frequency       == pdm->dmDisplayFrequency)))
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((1, "...Found a mode match!"));
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);

    }

    // If no mode has been found, return an error

    if (pVideoModeSelected == NULL)
    {
        DISPDBG((1, "...Couldn't find a mode match!"));
        EngFreeMem(pVideoBuffer);
        goto ReturnFalse;
    }

    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    VideoModeInformation = *pVideoModeSelected;
    EngFreeMem(pVideoBuffer);

    #if DEBUG_HEAP
        VideoModeInformation.VisScreenWidth  = 640;
        VideoModeInformation.VisScreenHeight = 480;
        pdm->dmPelsWidth = 640;
        pdm->dmPelsHeight = 480;
    #endif

    // Set up screen information from the mini-port:

    ppdev->ulMode           = VideoModeInformation.ModeIndex;
    ppdev->cxScreen         = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen         = VideoModeInformation.VisScreenHeight;
    ppdev->cBitsPerPel      = VideoModeInformation.BitsPerPlane;

    DISPDBG((1, "ScreenStride: %lx", VideoModeInformation.ScreenStride));

    // We handle HOOK_SYNCHRONIZE separately at surface creation time:

    ppdev->flHooks          = (HOOK_BITBLT         |
                               HOOK_TEXTOUT        |
                               HOOK_FILLPATH       |
                               HOOK_COPYBITS       |
                               HOOK_STROKEPATH     |
                               HOOK_LINETO         |
                               HOOK_STRETCHBLT     |
                               HOOK_TRANSPARENTBLT);

    // Fill in the GDIINFO data structure with the default 8bpp values:

    *pgdi = ggdiDefault;

    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:

    pgdi->ulHorzSize        = VideoModeInformation.XMillimeter;
    pgdi->ulVertSize        = VideoModeInformation.YMillimeter;

    pgdi->ulHorzRes         = VideoModeInformation.VisScreenWidth;
    pgdi->ulVertRes         = VideoModeInformation.VisScreenHeight;
    pgdi->ulPanningHorzRes  = VideoModeInformation.VisScreenWidth;
    pgdi->ulPanningVertRes  = VideoModeInformation.VisScreenHeight;

    pgdi->cBitsPixel        = VideoModeInformation.BitsPerPlane;
    pgdi->cPlanes           = VideoModeInformation.NumberOfPlanes;
    pgdi->ulVRefresh        = VideoModeInformation.Frequency;

    pgdi->ulDACRed          = VideoModeInformation.NumberRedBits;
    pgdi->ulDACGreen        = VideoModeInformation.NumberGreenBits;
    pgdi->ulDACBlue         = VideoModeInformation.NumberBlueBits;

    pgdi->ulLogPixelsX      = pdm->dmLogPixels;
    pgdi->ulLogPixelsY      = pdm->dmLogPixels;

    // Fill in the devinfo structure with the default 8bpp values:

    *pdi = gdevinfoDefault;

    if (VideoModeInformation.BitsPerPlane == 8)
    {
        ppdev->cjPelSize       = 1;
        ppdev->iBitmapFormat   = BMF_8BPP;

        // Assuming palette is orthogonal - all colors are same size.

        ppdev->cPaletteShift   = 8 - pgdi->ulDACRed;
        DISPDBG((3, "palette shift = %d\n", ppdev->cPaletteShift));
    }
    else if ((VideoModeInformation.BitsPerPlane == 16) ||
             (VideoModeInformation.BitsPerPlane == 15))
    {
        ppdev->cjPelSize       = 2;
        ppdev->iBitmapFormat   = BMF_16BPP;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_16BPP;

        pdi->iDitherFormat     = BMF_16BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else if (VideoModeInformation.BitsPerPlane == 24)
    {
        ppdev->cjPelSize       = 3;
        ppdev->iBitmapFormat   = BMF_24BPP;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_24BPP;

        pdi->iDitherFormat     = BMF_24BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else
    {
        ASSERTDD(VideoModeInformation.BitsPerPlane == 32,
         "This driver supports only 8, 16, 24 and 32bpp");

        ppdev->cjPelSize       = 4;
        ppdev->iBitmapFormat   = BMF_32BPP;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_32BPP;

        pdi->iDitherFormat     = BMF_32BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }

    DISPDBG((5, "Passed bInitializeModeFields"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bInitializeModeFields"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* DWORD getAvailableModes
*
* Calls the miniport to get the list of modes supported by the kernel driver,
* and returns the list of modes supported by the diplay driver among those
*
* returns the number of entries in the videomode buffer.
* 0 means no modes are supported by the miniport or that an error occured.
*
* NOTE: the buffer must be freed up by the caller.
*
\**************************************************************************/

DWORD getAvailableModes(
HANDLE                   hDriver,
PVIDEO_MODE_INFORMATION* modeInformation,       // Must be freed by caller
DWORD*                   cbModeSize)
{
    ULONG                   ulTemp;
    VIDEO_NUM_MODES         modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;

    //
    // Get the number of modes supported by the mini-port
    //

    if (EngDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                           NULL,
                           0,
                           &modes,
                           sizeof(VIDEO_NUM_MODES),
                           &ulTemp))
    {
        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_NUM_AVAIL_MODES"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = EngAllocMem(FL_ZERO_MEMORY,
                                   modes.NumModes * modes.ModeInformationLength,
                                   ALLOC_TAG);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "getAvailableModes - Failed EngAllocMem"));
        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (EngDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_AVAIL_MODES,
                           NULL,
                           0,
                           *modeInformation,
                           modes.NumModes * modes.ModeInformationLength,
                           &ulTemp))
    {

        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_AVAIL_MODES"));

        EngFreeMem(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

        return(0);
    }

    //
    // Now see which of these modes are supported by the display driver.
    // As an internal mechanism, set the length to 0 for the modes we
    // DO NOT support.
    //

    ulTemp = modes.NumModes;
    pVideoTemp = *modeInformation;

    //
    // Mode is rejected if it is not one plane, or not graphics, or is not
    // one of 8, 15, 16, 24 or 32 bits per pel.
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((pVideoTemp->BitsPerPlane != 8) &&
             (pVideoTemp->BitsPerPlane != 15) &&
             (pVideoTemp->BitsPerPlane != 16) &&
             (pVideoTemp->BitsPerPlane != 24) &&
             (pVideoTemp->BitsPerPlane != 32)))
        {
            DISPDBG((2, "Rejecting miniport mode:"));
            DISPDBG((2, "   Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((2, "   Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((2, "   Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                   pVideoTemp->NumberOfPlanes));
            DISPDBG((2, "   Frequency     -- %li", pVideoTemp->Frequency));

            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return(modes.NumModes);
}
