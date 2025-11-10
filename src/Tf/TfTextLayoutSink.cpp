#include "FanyDefines.h"
#include "Private.h"
#include "TfTextLayoutSink.h"
#include "MetasequoiaIME.h"
#include "GetTextExtentEditSession.h"

CTfTextLayoutSink::CTfTextLayoutSink(_In_ CMetasequoiaIME *pTextService)
{
    _pTextService = pTextService;
    _pTextService->AddRef();

    _pRangeComposition = nullptr;
    _pContextDocument = nullptr;
    _tfEditCookie = TF_INVALID_EDIT_COOKIE;

    _dwCookieTextLayoutSink = TF_INVALID_COOKIE;

    _refCount = 1;

    DllAddRef();
}

CTfTextLayoutSink::~CTfTextLayoutSink()
{
    if (_pTextService)
    {
        _pTextService->Release();
    }

    DllRelease();
}

STDAPI CTfTextLayoutSink::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextLayoutSink))
    {
        *ppvObj = (ITfTextLayoutSink *)this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CTfTextLayoutSink::AddRef()
{
    return ++_refCount;
}

STDAPI_(ULONG) CTfTextLayoutSink::Release()
{
    LONG cr = --_refCount;

    assert(_refCount >= 0);

    if (_refCount == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ITfTextLayoutSink::OnLayoutChange
//
//----------------------------------------------------------------------------

STDAPI CTfTextLayoutSink::OnLayoutChange(_In_ ITfContext *pContext, TfLayoutCode lcode,
                                         _In_ ITfContextView *pContextView)
{
    // we're interested in only document context.
    if (pContext != _pContextDocument)
    {
        return S_OK;
    }

    switch (lcode)
    {
    case TF_LC_CREATE: {
#ifdef FANY_DEBUG
        // TODO: Log TF_LC_CREATE is triggered
#endif
    }
    case TF_LC_CHANGE: {
        CGetTextExtentEditSession *pEditSession = nullptr;
        pEditSession = new (std::nothrow)
            CGetTextExtentEditSession(_pTextService, pContext, pContextView, _pRangeComposition, this);
        if (nullptr != (pEditSession))
        {
            HRESULT hr = S_OK;
            pContext->RequestEditSession(_pTextService->_GetClientId(), pEditSession, TF_ES_SYNC | TF_ES_READ, &hr);
#ifdef FANY_DEBUG
            // TODO: Log TF_LC_CHANGE is triggered and edit session is started
#endif
            pEditSession->Release();
        }
    }
    break;

    case TF_LC_DESTROY:
        _LayoutDestroyNotification();
        break;
    }
    return S_OK;
}

HRESULT CTfTextLayoutSink::_StartLayout(_In_ ITfContext *pContextDocument, TfEditCookie ec,
                                        _In_ ITfRange *pRangeComposition)
{
    _pContextDocument = pContextDocument;
    _pContextDocument->AddRef();

    _pRangeComposition = pRangeComposition;
    _pRangeComposition->AddRef();

    _tfEditCookie = ec;

    return _AdviseTextLayoutSink();
}

VOID CTfTextLayoutSink::_EndLayout()
{
    if (_pRangeComposition)
    {
        _pRangeComposition->Release();
        _pRangeComposition = nullptr;
    }

    if (_pContextDocument)
    {
        _UnadviseTextLayoutSink();
        _pContextDocument->Release();
        _pContextDocument = nullptr;
    }
}

HRESULT CTfTextLayoutSink::_AdviseTextLayoutSink()
{
    HRESULT hr = S_OK;
    ITfSource *pSource = nullptr;

    hr = _pContextDocument->QueryInterface(IID_ITfSource, (void **)&pSource);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pSource->AdviseSink(IID_ITfTextLayoutSink, (ITfTextLayoutSink *)this, &_dwCookieTextLayoutSink);
    if (FAILED(hr))
    {
        pSource->Release();
        return hr;
    }

    pSource->Release();

    return hr;
}

HRESULT CTfTextLayoutSink::_UnadviseTextLayoutSink()
{
    HRESULT hr = S_OK;
    ITfSource *pSource = nullptr;

    if (nullptr == _pContextDocument)
    {
        return E_FAIL;
    }

    hr = _pContextDocument->QueryInterface(IID_ITfSource, (void **)&pSource);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pSource->UnadviseSink(_dwCookieTextLayoutSink);
    if (FAILED(hr))
    {
        pSource->Release();
        return hr;
    }

    pSource->Release();

    return hr;
}

/**
 * @brief 获取 caret 的坐标
 *
 * 本质上是通过 _pContextView->GetTextExt 获取 caret 坐标，
 * 因此，涉及到 caret 坐标的地方不止这里，还有其他地方，是直接
 * 使用 _pContextView->GetTextExt 来获取坐标的。
 *
 * @param lpRect
 * @return HRESULT
 */
HRESULT CTfTextLayoutSink::_GetTextExt(_Out_ RECT *lpRect)
{
    HRESULT hr = S_OK;
    BOOL isClipped = TRUE;
    ITfContextView *pContextView = nullptr;

    hr = _pContextDocument->GetActiveView(&pContextView);
    if (FAILED(hr))
    {
        return hr;
    }

    if (FAILED(hr = pContextView->GetTextExt(_tfEditCookie, _pRangeComposition, lpRect, &isClipped)))
    {
#ifdef FANY_DEBUG
        // TODO: Log GetTextExt failed
#endif
        // Set default value to make sure the window is hidden by moving it out of the screen
        lpRect->left = 0;
        lpRect->bottom = Global::INVALID_Y;
    }
#ifdef FANY_DEBUG
    // TODO: Log lpRect position
#endif

    pContextView->Release();

    return S_OK;
}
