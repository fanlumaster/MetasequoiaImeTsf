#include "Private.h"
#include "EditSession.h"
#include "GetTextExtentEditSession.h"
#include "TfTextLayoutSink.h"
#include "Ipc.h"
#include "FanyDefines.h"

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CGetTextExtentEditSession::CGetTextExtentEditSession(_In_ CMetasequoiaIME *pTextService, _In_ ITfContext *pContext,
                                                     _In_ ITfContextView *pContextView,
                                                     _In_ ITfRange *pRangeComposition,
                                                     _In_ CTfTextLayoutSink *pTfTextLayoutSink)
    : CEditSessionBase(pTextService, pContext)
{
    _pContextView = pContextView;
    _pRangeComposition = pRangeComposition;
    _pTfTextLayoutSink = pTfTextLayoutSink;
}

//+---------------------------------------------------------------------------
//
// ITfEditSession::DoEditSession
//
//----------------------------------------------------------------------------

STDAPI CGetTextExtentEditSession::DoEditSession(TfEditCookie ec)
{
    RECT rc = {0, 0, 0, 0};
    BOOL isClipped = TRUE;

    if (SUCCEEDED(_pContextView->GetTextExt(ec, _pRangeComposition, &rc, &isClipped)))
    {
#ifdef FANY_DEBUG
        // TODO: Log rc position
#endif
        Global::Point[0] = rc.left * Global::DpiScale;
        Global::Point[1] = rc.bottom * Global::DpiScale;
        if (Global::current_process_name == Global::ZEN_BROWSER)
        {
            Global::firefox_like_cnt++;
            if (Global::firefox_like_cnt == 3)
            {
                _pTfTextLayoutSink->_LayoutChangeNotification(&rc);
            }
        }
        else
        {
            _pTfTextLayoutSink->_LayoutChangeNotification(&rc);
        }
    }

    return S_OK;
}
