#pragma once

#include "Private.h"

class CMetasequoiaIME;

class CTfTextLayoutSink : public ITfTextLayoutSink
{
  public:
    CTfTextLayoutSink(_In_ CMetasequoiaIME *pTextService);
    virtual ~CTfTextLayoutSink();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, _Outptr_ void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfTextLayoutSink
    STDMETHODIMP OnLayoutChange(_In_ ITfContext *pContext, TfLayoutCode lcode, _In_ ITfContextView *pContextView);

    HRESULT _StartLayout(_In_ ITfContext *pContextDocument, TfEditCookie ec, _In_ ITfRange *pRangeComposition);
    VOID _EndLayout();

    HRESULT _GetTextExt(_Out_ RECT *lpRect);
    ITfContext *_GetContextDocument()
    {
        return _pContextDocument;
    };

    virtual VOID _LayoutChangeNotification(_In_ RECT *lpRect) = 0;
    virtual VOID _LayoutDestroyNotification() = 0;

  private:
    HRESULT _AdviseTextLayoutSink();
    HRESULT _UnadviseTextLayoutSink();

  private:
    ITfRange *_pRangeComposition;
    ITfContext *_pContextDocument;
    TfEditCookie _tfEditCookie;
    CMetasequoiaIME *_pTextService;
    DWORD _dwCookieTextLayoutSink;
    LONG _refCount;
};
