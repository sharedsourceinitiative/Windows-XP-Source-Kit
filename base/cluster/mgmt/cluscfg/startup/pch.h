//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      pch.h
//
//  Description:
//      Precompiled header file for the EvictCleanup library.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#define _UNICODE
#define UNICODE

#if DBG==1 || defined( _DEBUG )
#define DEBUG
#define USES_SYSALLOCSTRING
#endif


////////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For the windows API and types
#include <windows.h>

// For COM
#include <objbase.h>
#include <ComCat.h>

// Required to be a part of this DLL
#include <Common.h>
#include <Debug.h>
#include <Log.h>
#include <CITracker.h>
#include <CFactory.h>
#include <Dll.h>
#include <Guids.h>
#include <ClusCfgGuids.h>
#include <ClusCfgServer.h>
