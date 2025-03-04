//-----------------------------------------------------------------------------

//  

//  File: goto.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once

class LTAPIENTRY CGoto : public CRefCount
{
public:
	CGoto()	{};
	
	virtual void Edit() = 0;
	virtual BOOL Go() = 0;


private:
	CGoto(const CGoto &);
	
};

#pragma warning(disable:4251)

class LTAPIENTRY CShellGoto : public CGoto
{
public:
	CShellGoto(const TCHAR *szFileName);

	virtual void Edit();
	virtual BOOL Go();

private:

	CLString m_strFileName;
};

#pragma warning(default:4251)
