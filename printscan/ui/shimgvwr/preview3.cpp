// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.


#include "stdafx.h"
#include "preview3.h"

/////////////////////////////////////////////////////////////////////////////
// CPreview3

IMPLEMENT_DYNCREATE(CPreview3, CWnd)

/////////////////////////////////////////////////////////////////////////////
// CPreview3 properties

/////////////////////////////////////////////////////////////////////////////
// CPreview3 operations

void CPreview3::ShowFile(LPCTSTR bstrFileName, long iSelectCount)
{
    static BYTE parms[] =
        VTS_BSTR VTS_I4;
    InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
         bstrFileName, iSelectCount);
}

long CPreview3::GetPrintable()
{
    long result;
    InvokeHelper(0x2, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void CPreview3::SetPrintable(long nNewValue)
{
    static BYTE parms[] =
        VTS_I4;
    InvokeHelper(0x2, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
         nNewValue);
}

long CPreview3::GetCxImage()
{
    long result;
    InvokeHelper(0x3, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

long CPreview3::GetCyImage()
{
    long result;
    InvokeHelper(0x4, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void CPreview3::Show(const VARIANT& var)
{
    static BYTE parms[] =
        VTS_VARIANT;
    InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
         &var);
}

void CPreview3::Zoom(long iSelectCount)
{
    static BYTE parms[] =
        VTS_I4;
    InvokeHelper(0x60030000, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
         iSelectCount);
}

void CPreview3::BestFit()
{
    InvokeHelper(0x60030001, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void CPreview3::ActualSize()
{
    InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void CPreview3::SlideShow()
{
    InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void CPreview3::Rotate(unsigned long dwAngle)
{
    static BYTE parms[] =
        VTS_I4;
    InvokeHelper(0x60040000, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
         dwAngle);
}

void CPreview3::IsValidVerb(LPCTSTR bstrVerb)
{
    static BYTE parms[] =
        VTS_BSTR;
    InvokeHelper(0x60040001, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
         bstrVerb);
}

void CPreview3::Commit()
{
    InvokeHelper(0x60040002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void CPreview3::SaveAs(LPCTSTR bstrPath)
{
    static BYTE parms[] =
        VTS_BSTR;
    
    
        InvokeHelper(0x60040003, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
                     bstrPath);
    
    
    
    
}
