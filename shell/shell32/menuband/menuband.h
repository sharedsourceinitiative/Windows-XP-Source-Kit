#ifndef _MENUBAND_H_
#define _MENUBAND_H_

#include "bands.h"
#include "mnbase.h"

#include "legacy.h"

// Flags for MBANDCID_POPUPITEM

#define MBPUI_SETITEM           0x00001
#define MBPUI_INITIALSELECT     0x00002
#define MBPUI_ITEMBYPOS      0x00004

#ifdef STARTMENUSPLIT
// Flags for constructor
#define MENUBAND_HORIZ      0x00000001
#define MENUBAND_TOPLEVEL   0x00000002
#endif

// Special indices for MBANDCID_SELECTITEM
#define MBSI_FIRSTITEM       0
#define MBSI_NONE           -1
#define MBSI_LASTITEM       -2

// This arrow is used when we are in Right-To-Left Mirror mode
#define CH_MENUARROWRTLA '3'

// Forward declare
struct CMBMsgFilter;

// Define this to get Shell Expando menu style
// Undefine to get "Office IntelliMenu" style
//#define DRAWEDGE

// The CMenuBand class handles all menu behavior for bands.  

class CMenuBandMetrics : public IUnknown
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    HFONT   _CalcFont(HWND hwnd, LPCTSTR pszFont, DWORD dwCharSet, TCHAR ch, int* pcx, 
        int* pcy, int* pcxMargin, int iOrientation, int iWeight);
    void    _SetMenuFont();         // (Called for TopLevMBand only) Sets: _hFontMenu
    void    _SetArrowFont(HWND hwnd);        // (Called for TopLevMBand only) Sets: _hFontArrow, _cyArrow
    void    _SetChevronFont(HWND hwnd);
    void    _SetTextBrush(HWND hwnd);

#ifndef DRAWEDGE
    void    _SetPaintMetrics(HWND hwnd);
#endif
    void    _SetColors();
    int     _cyArrow;               // Height of cascade arrow
    int     _cxArrow;               // Width of cascade arrow
    int     _cxMargin;              // Margin b/t text and arrow
    int     _cyChevron;
    int     _cxChevron;
    int     _cxChevronMargin;
    HCURSOR _hCursorOld;            // Cursor that was in use prior to entering menu mode
    HFONT   _hFontMenu;             // Font for menu text
    HFONT   _hFontArrow;            // Correct sized Marlett font for cascade arrow
    HFONT   _hFontChevron;
    HBRUSH  _hbrText;
#ifndef DRAWEDGE
    HPEN    _hPenHighlight;            // Pen for BTNHIGHLIGHT
    HPEN    _hPenShadow;               // Pen for BTNSHADOW
#endif

    COLORREF _clrBackground;
    COLORREF _clrDemoted;
    COLORREF _clrMenuText;
    COLORREF _clrMenuGrad;

    BOOL  _fHighContrastMode;        // Accessibility
    BOOL  _fFlatMenuMode;
    BOOL  _fInit;

    CMenuBandMetrics();
    void Init(HWND hwnd);

private:
    ~CMenuBandMetrics();

    ULONG _cRef;
};


class CMenuBandState
{

    // Global State Variables
    BYTE    _fsUEMState;
    BOOL    _fKeyboardCue;
    IFadeTask* _ptFader;
    IShellTaskScheduler* _pScheduler;
    HWND    _hwndSubclassed;
    HWND    _hwndToolTip;
    HWND    _hwndWorker;
    void*   _pvContext;

    // Bits
    BITBOOL _fExpand: 1;
    BITBOOL _fContainsDrag: 1;
    BITBOOL _fTipShown: 1;
    BITBOOL _fBalloonStyle: 1;

    int     _cChangeNotify;

    // This will, in the future, contain the menuband stack
public:
    CMenuBandState();
    virtual ~CMenuBandState();

    // Set/Get the expand state for new popups.
    BOOL GetExpand()                { return (BOOL)_fExpand;      };
    void SetExpand(BOOL fExpand)    { _fExpand = BOOLIFY(fExpand);};
    BYTE GetUEMState()              { return _fsUEMState;         };
    void SetUEMState(BYTE bState)   { _fsUEMState = bState;       };
    BOOL GetKeyboardCue();
    void SetKeyboardCue(int iKC);
    BOOL HasDrag()                  { return _fContainsDrag;      };
    void HasDrag(BOOL fHasDrag)     { _fContainsDrag = BOOLIFY(fHasDrag); };
    void SetSubclassedHWND(HWND hwndSubclassed)   
                                    { _hwndSubclassed = hwndSubclassed; };
    HWND GetSubclassedHWND()        { return _hwndSubclassed; };
    HWND GetWorkerWindow(HWND hwndParent);
    void PushChangeNotify()         { ++_cChangeNotify; };
    void PopChangeNotify()          { --_cChangeNotify; };
    BOOL IsProcessingChangeNotify() { return _cChangeNotify != 0;   };

    // Context Stuff
    // a menuband context is used for the global message filter. Since we may have
    // more than one menu present in the system, there is a race condition that can occur
    // where the menu in one thread tries to pop off it's menus, when a menuband in another thread
    // tries to push them on. Through the use of a context, we can know when this is happening and
    // make sure that we pop all of the menus of one context before pushing from another.
    // - lamadio 9.15.98
    void SetContext(void* pvContext)    {_pvContext = pvContext;};
    void* GetContext()                  { return _pvContext;};

    // Fade Stuff
    HRESULT FadeRect(LPCRECT prc);
    void CreateFader();
    IShellTaskScheduler* GetScheduler();

    // Chevron Tip Stuff
    void CenterOnButton(HWND hwndTB, BOOL fBalloon, int idCmd, LPTSTR pszTitle, LPTSTR szTip);
    void HideTooltip(BOOL fAllowBalloonCollapse);
    void PutTipOnTop();
};



interface IShellMenuAcc: public IUnknown
{
    // *** IShellMenuAcc methods ***
    STDMETHOD(GetTop)(THIS_ CMenuToolbarBase** ppmtbTop) PURE;
    STDMETHOD(GetBottom)(THIS_ CMenuToolbarBase** ppmtbBottom) PURE;
    STDMETHOD(GetTracked)(THIS_ CMenuToolbarBase** ppmtbTracked) PURE;
    STDMETHOD(GetParentSite)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD(GetState)(THIS_ BOOL* pfVertical, BOOL* pfOpen) PURE;
    STDMETHOD(DoDefaultAction)(THIS_ VARIANT* pvarChild) PURE;
    STDMETHOD(GetSubMenu)(THIS_ VARIANT* pvarChild, REFIID riid, void** ppvObj) PURE;
    STDMETHOD(IsEmpty)() PURE;
};

// {FAF6FE96-CE5E-11d1-8371-00C04FD918D0}
DEFINE_GUID(IID_IShellMenuAcc,              0xfaf6fe96, 0xce5e, 0x11d1, 0x83, 0x71, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0);



class CMenuBand : public CToolBand,
                  public IMenuPopup,
                  public IMenuBand,
                  public IShellMenu2,
                  public IWinEventHandler,
                  public IShellMenuAcc
{
    // REVIEW (lamadio): I don't like this. Should I make these nested classes?
    friend class CMenuToolbarBase;
    friend class CMenuSFToolbar;
    friend class CMenuStaticToolbar;
    friend class CToolbarMenu;
    friend struct CMBMsgFilter;

public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) 
        { return CToolBand::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void)
        { return CToolBand::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDockingWindow methods (override) ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);
    
    // *** IInputObject methods (override) ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL bEnterMode);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IDeskBar methods ***
    virtual STDMETHODIMP SetClient(IUnknown* punk) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetClient(IUnknown** ppunkClient) { return E_NOTIMPL; };
    virtual STDMETHODIMP OnPosRectChangeDB (LPRECT prc);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IMenuPopup methods ***
    virtual STDMETHODIMP OnSelect(DWORD dwSelectType);
    virtual STDMETHODIMP SetSubMenu(IMenuPopup* pmp, BOOL fSet);
    virtual STDMETHODIMP Popup(POINTL *ppt, RECTL *prcExclude, DWORD dwFlags) { return E_NOTIMPL; };

    // *** IMenuBand methods ***
    virtual STDMETHODIMP IsMenuMessage(MSG * pmsg);
    virtual STDMETHODIMP TranslateMenuMessage(MSG * pmsg, LRESULT * plRet);

    // *** IShellMenu methods ***
    virtual STDMETHODIMP Initialize(IShellMenuCallback* psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags);
    virtual STDMETHODIMP GetMenuInfo(IShellMenuCallback** ppsmc, UINT* puId, 
                                    UINT* puIdAncestor, DWORD* pdwFlags);
    virtual STDMETHODIMP SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HKEY hkey, DWORD dwFlags);
    virtual STDMETHODIMP GetShellFolder(DWORD* pdwFlags, LPITEMIDLIST* ppidl, REFIID riid, void** ppvObj);
    virtual STDMETHODIMP SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    virtual STDMETHODIMP GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags);
    virtual STDMETHODIMP InvalidateItem(LPSMDATA psmd, DWORD dwFlags);
    virtual STDMETHODIMP GetState(LPSMDATA psmd);
    virtual STDMETHODIMP SetMenuToolbar(IUnknown* punk, DWORD dwFlags);

    // *** IShellMenu2 methods ***
    virtual STDMETHODIMP GetSubMenu(UINT idCmd, REFIID riid, LPVOID *ppvObj);
    virtual STDMETHODIMP SetToolbar(HWND hwnd, DWORD dwFlags);
    virtual STDMETHODIMP SetMinWidth(int cxMenu);
    virtual STDMETHODIMP SetNoBorder(BOOL fNoBorder);
    virtual STDMETHODIMP SetTheme(LPCWSTR pszTheme);

    // ** IPersist ***
    virtual STDMETHODIMP GetClassID(CLSID*) { return E_NOTIMPL; };
    virtual STDMETHODIMP Load(IStream*) { return E_NOTIMPL; };
    virtual STDMETHODIMP Save(IStream*, BOOL) { return E_NOTIMPL; };

    // ** IWinEventHandler ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);


    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IOleCommandTarget ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                              DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                              VARIANTARG *pvarargOut);

    // *** IShellMenuAcc ***
    virtual STDMETHODIMP GetTop(CMenuToolbarBase** ppmtbTop);
    virtual STDMETHODIMP GetBottom(CMenuToolbarBase** ppmtbBottom);
    virtual STDMETHODIMP GetTracked(CMenuToolbarBase** ppmtbTracked);
    virtual STDMETHODIMP GetParentSite(REFIID riid, void** ppvObj);
    virtual STDMETHODIMP GetState(BOOL* pfVertical, BOOL* pfOpen);
    virtual STDMETHODIMP DoDefaultAction(VARIANT* pvarChild);
    virtual STDMETHODIMP GetSubMenu(VARIANT* pvarChild, REFIID riid, void** ppvObj);
    virtual STDMETHODIMP IsEmpty();
    
    // Other methods
    LRESULT GetMsgFilterCB(MSG * pmsg, BOOL bRemove);
    inline BOOL IsInSubMenu(void)       { return _fInSubMenu; };
    inline DWORD GetFlags()   {return _dwFlags; };
    
    // Other public methods
    void    ResizeMenuBar();        // Make our parent menubar resize
    void    SetTrackMenuPopup(IUnknown* punk);
    HRESULT ForwardChangeNotify(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);   // Change notify Forwarder.

    CMenuBand();


    BOOL    SetTracked(CMenuToolbarBase* pmtb);

private:

    friend HRESULT CMenuBand_CreateInstance(IUnknown * punkOuter, REFIID riid, void **ppv);
    friend VOID CALLBACK CMenuBand_TimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime );
    friend CMenuBand * CMenuBand_Create(IShellFolder* psf, LPCITEMIDLIST pidl,
                             BOOL bHorizontal);


    virtual ~CMenuBand();

    HRESULT _Initialize(DWORD dwFlags);// Flags are MENUBAND_*

    HRESULT _EnterMenuMode(void);
    void    _ExitMenuMode(void);
    void    _GetFontMetrics();      // (Called for non-TopLevelMBands) Uses IUnk_QS to set
                                    //  _hFontMenu, _hFontArrow, _cyArrow based on TopLevel's values
    void    _CancelMode(DWORD dwType);
    HRESULT _AddToolbar(CMenuToolbarBase* pmtb, DWORD dwFlags);
    HRESULT _SiteOnSelect(DWORD dwType);
    HRESULT _SubMenuOnSelect(DWORD dwType);

    HRESULT _OnSysChar(MSG * pmsg, BOOL bFirstDibs);
    HRESULT _HandleAccelerators(MSG * pmsg);

    HRESULT _SiteSetSubMenu(IMenuPopup * pmp, BOOL bSet);

    void    _OnSelectArrow(int iDir);
    void    _UpdateButtons();

    HRESULT _CallCB(DWORD dwMsg, WPARAM wParam = 0, LPARAM lParam = 0);
    HRESULT _ProcessMenuPaneMessages(MSG* pmsg);

    
    // Member variables
    int     _nItemCur;              // Current item selected
    int     _nItemNew;              // New item to be selected/expanded
    int     _nItemTimer;            // the timer id for popping up cascading menus
    int     _nItemMove;             // Current item being moved
    int     _nItemSubMenu;          // item that's cascaded
    HWND    _hwndFocusPrev;
    HWND    _hwndParent;
    BOOL    _fNoBorder;
    WCHAR*  _pszTheme;

    CMenuToolbarBase*   _pmtbMenu;          // The static menu toolbar 
                                            //  (may be pmtbTop or pmtbBottom)
    CMenuToolbarBase*   _pmtbShellFolder;   // Non-menu toolbar (shellfolder or other)
                                            //  (may be pmtbTop or pmtbBottom)
    
    CMenuToolbarBase*   _pmtbTop;           // The top toolbar
    CMenuToolbarBase*   _pmtbBottom;        // The bottom toolbar (may be == pmtbTop)
    CMenuToolbarBase*   _pmtbTracked;       // This is transient, may be pmtbTop or pmtbBottom

    IAugmentedShellFolder2* _pasf2;

    UINT    _uId;       // Id of this band (Derived from the item that poped it up)
    UINT    _uIdAncestor;   // Id of the top most menu item.
    HMENU   _hmenu;
    HWND    _hwndMenuOwner;
    DWORD   _dwMenuFlags;


    CMenuBandState*     _pmbState;              // Menu Band global state
    CMenuBandMetrics*   _pmbm;
    IShellMenuCallback* _psmcb;             // Callback Mechanism
    HCURSOR             _hCursorOld;
    DWORD               _dwFlags;
    UINT                _uIconSize;

    IMenuPopup*         _pmpSubMenu;        // Cached submenu
    IMenuPopup*         _pmpTrackPopup;     // Find a way to use only menubands.

    void*               _pvUserData;        // User associated data.


    BITBOOL _fTopLevel: 1;          // TRUE: this is the toplevel parent menu
                                    //       (may be vertical for context menus)
    BITBOOL _fMenuMode: 1;          // TRUE: we are in menu mode
    BITBOOL _fPopupNewMenu: 1;      // TRUE: popup a new menu (_nItemNew) 
                                    //    once done with current menu
    BITBOOL _fInitialSelect: 1;     // TRUE: select first item when popping up submenu
    BITBOOL _fInSubMenu: 1;         // TRUE: currently in a submenu
    BITBOOL _fAltSpace: 1;          // TRUE: Alt-space was hit
    BITBOOL _fMenuFontCreated: 1;   // TRUE: This instance created the font and should delete it
    BITBOOL _fArrowFontCreated: 1;  // TRUE: This instance created the font and should delete it
    BITBOOL _fEmpty: 1;             // TRUE: Menu is empty
    BITBOOL _fParentIsNotASite: 1;  
    BITBOOL _fKeyboardSelected: 1;  
    BITBOOL _fInvokedByDrag: 1;     // TRUE: the menu cascaded open b/c of drag/drop
    BITBOOL _fDragEntered : 1;
    BITBOOL _fSysCharHandled: 1;    // TRUE: WM_SYSCHAR was already handled
    BITBOOL _fHasSubMenu:1;
    BITBOOL _fAppActive:1;       // TRUE: The Menus should be drawn using Memphis' grey menu.
    BITBOOL _fVertical: 1;
    BITBOOL _fShow : 1;
    BITBOOL _fClosing: 1;           // TRUE: When CloseDW is called.
    BITBOOL _fForceButtonUpdate: 1; // TRUE: Force a v_UpdateButtons.
    BITBOOL _fProcessingDup: 1;     // TRUE: when the contained classes are processing Dup chars.
    BITBOOL _fExpanded: 1;          // TRUE: This band is expanded
    BITBOOL _fCascadeAnimate: 1;
    BITBOOL _fPopupNewItemOnShow: 1;    // Causes _nItemNew to be displayed at ShowDW time.
    BITBOOL _fParentIsHorizontal: 1;

#ifdef DEBUG
    BITBOOL _fInitialized: 1;
#endif
    

    DEBUG_CODE( int _nMenuLevel; )
};

int MsgFilter_GetCount();
HRESULT IUnknown_BandChildSetKey(IUnknown* punk, HKEY hKey);

// The message filter redirects messages to the menuband
// that is at the top of the stack.  Each additional cascade
// pushes the new menuband onto the stack, and a cancel-level
// pops one off.
typedef struct tagMBELEM
{
    CMenuBand * pmb;
    HWND        hwndTB;
    HWND        hwndBar;
    RECT        rc;             // rect of hwndBar
    BITBOOL     bInitRect: 1;   // TRUE: rc is initialized
} MBELEM;

#define CMBELEM_INIT    10
#define CMBELEM_GROW    10

struct CMBMsgFilter
{
public:
    HHOOK       _hhookMsg;
    HWND        _hwndCapture;
    FDSA        _fdsa;              // Stack
    MBELEM      _rgmbelem[CMBELEM_INIT];
    BITBOOL     _fPreventCapture : 1;
    BITBOOL     _fInitialized: 1;
    BITBOOL     _fSetAtPush: 1;
    BITBOOL     _fDontIgnoreSysChar: 1;
    BITBOOL     _fEngaged: 1;
    BITBOOL     _fModal: 1;
    BITBOOL     _fAllocated: 1;
    POINT       _ptLastMove;
    CMenuBand*  _pmb;
    int         _iSysCharStack;
    void*       _pvContext;
    HCURSOR     _hcurArrow;
    int         _cRef;

    DEBUG_CODE( int _nMenuLevel; )
    void    AddRef();
    void    Release();


    void    ForceModalCollapse();
    void    SetModal(BOOL fModal);      // This is so that
                                        // a modal message band (Links)
                                        // sets activation correctly.
    void*   GetContext()    {   return _pvContext;  };
    void    SetContext(void* pvContext, BOOL fSet);
    void    Push(void* pvContext, CMenuBand * pmb, IUnknown * punkSite);
    int     Pop(void* pvContext);
    void    RetakeCapture(void);
    void    AcquireMouseLocation()   { GetCursorPos(&_ptLastMove); };


    void    ReEngage(void* pvContext);
    void    DisEngage(void* pvContext);
    BOOL    IsEngaged()  { return _fEngaged;};

    CMenuBand * _GetBottomMostSelected(void);
    CMenuBand * _GetTopPtr(void);
    CMenuBand * _GetWindowOwnerPtr(HWND hwnd);
    CMenuBand * _HitTest(POINT pt, HWND * phwnd = NULL);
    LRESULT     _HandleMouseMsgs(MSG * pmsg, BOOL bRemove);
    HRESULT     _TopFilterMouseMessage(MSG *pmsg, BOOL bRemove, CMenuBand *pmbTarget);
    int         GetCount();
    void        PreventCapture(BOOL bSet) { _fPreventCapture = BOOLIFY(bSet); }
    CMenuBand * GetTopMostPtr(void)         { return _pmb; };
    void        SetTopMost(CMenuBand* pmb)  {_pmb = pmb; };
    void        SetHook(BOOL fSet, BOOL fIgnoreSysChar); 

    static LRESULT CALLBACK GetMsgHook(int nCode, WPARAM wParam, LPARAM lParam);

//private:
};

CMBMsgFilter* GetMessageFilter();


extern CMBMsgFilter g_msgfilter;
extern UINT    g_nMBPopupOpen;
extern UINT    g_nMBFullCancel;
extern UINT    g_nMBDragCancel;
extern UINT    g_nMBAutomation;
extern UINT    g_nMBExecute;
extern UINT    g_nMBOpenChevronMenu;


#define MBTIMER_POPOUT      0x00008001         // event ID for popout menu timer
#define MBTIMER_DRAGOVER    0x00008002         // event ID for popout menu timer
#define MBTIMER_EXPAND      0x00008003
#define MBTIMER_TIMEOUT     (GetDoubleClickTime() * 4 / 5) // same formula that USER uses

#define MBTIMER_ENDEDIT     0x00008004
#define MBTIMER_ENDEDITTIME 1000

#define MBTIMER_CLOSE       0x00008005
#define MBTIMER_CLOSETIME   2000

#define MBTIMER_CLICKUNHANDLE 0x00008006

// Flashing Support
#define MBTIMER_FLASH       0x00008007
#define MBTIMER_FLASHTIME   100
#define COUNT_ENDFLASH      8

// UEM Profiling.
#define MBTIMER_UEMTIMEOUT  0x00008008

#define MBTIMER_DRAGPOPDOWN 0x00008009
#define MBTIMER_DRAGPOPDOWNTIMEOUT     (2 * GetDoubleClickTime()) //ASSERT(MBTIMER_DRAGPOPDOWNTIMEOUT > 
                                                        // MBTIMER_TIMEOUR)

#define MBTIMER_CHEVRONTIP  0x0000800A

#define MBTIMER_INFOTIP     0x0000800B
#define CH_RETURN       0xd

#define szfnMarlett     TEXT("MARLETT")

#endif  // _MENUBAND_H_
