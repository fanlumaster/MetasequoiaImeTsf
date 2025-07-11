#include "Private.h"
#include "Globals.h"
#include "EditSession.h"
#include "MetasequoiaIME.h"
#include "CandidateListUIPresenter.h"
#include "CompositionProcessorEngine.h"
#include "MetasequoiaIMEBaseStructure.h"
#include <debugapi.h>
#include <minwindef.h>
#include <string>
#include <fmt/xchar.h>
#include "FanyUtils.h"
#include "Ipc.h"

//////////////////////////////////////////////////////////////////////
//
// CMetasequoiaIME class
//
//////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// _IsRangeCovered
//
// Returns TRUE if pRangeTest is entirely contained within pRangeCover.
//
//----------------------------------------------------------------------------

BOOL CMetasequoiaIME::_IsRangeCovered(TfEditCookie ec, _In_ ITfRange *pRangeTest, _In_ ITfRange *pRangeCover)
{
    LONG lResult = 0;
    ;

    if (FAILED(pRangeCover->CompareStart(ec, pRangeTest, TF_ANCHOR_START, &lResult)) || (lResult > 0))
    {
        return FALSE;
    }

    if (FAILED(pRangeCover->CompareEnd(ec, pRangeTest, TF_ANCHOR_END, &lResult)) || (lResult < 0))
    {
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _DeleteCandidateList
//
//----------------------------------------------------------------------------

VOID CMetasequoiaIME::_DeleteCandidateList(BOOL isForce, _In_opt_ ITfContext *pContext)
{
    isForce;
    pContext;

    CCompositionProcessorEngine *pCompositionProcessorEngine = nullptr;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;
    pCompositionProcessorEngine->PurgeVirtualKey();

    if (_pCandidateListUIPresenter)
    {
        _pCandidateListUIPresenter->_EndCandidateList();

        _candidateMode = CANDIDATE_NONE;
        _isCandidateWithWildcard = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
// _HandleComplete
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleComplete(TfEditCookie ec, _In_ ITfContext *pContext)
{
    _DeleteCandidateList(FALSE, pContext);

    // just terminate the composition
    _TerminateComposition(ec, pContext);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCancel
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCancel(TfEditCookie ec, _In_ ITfContext *pContext)
{
    _RemoveDummyCompositionForComposing(ec, _pComposition);

    _DeleteCandidateList(FALSE, pContext);

    _TerminateComposition(ec, pContext);

    return S_OK;
}

HRESULT CMetasequoiaIME::_HandleToogleIMEMode(TfEditCookie ec, _In_ ITfContext *pContext)
{
    CStringRange keyStrokebuffer = _pCompositionProcessorEngine->GetKeystrokeBuffer();
    if (keyStrokebuffer.GetLength())
    {
        std::wstring keyStrokeString(keyStrokebuffer.Get(), keyStrokebuffer.GetLength());
        FanyUtils::SendKeys(keyStrokeString);
    }
    _RemoveDummyCompositionForComposing(ec, _pComposition);

    _DeleteCandidateList(FALSE, pContext);

    _TerminateComposition(ec, pContext);

    // CCompositionProcessorEngine *pCompositionProcessorEngine;
    // pCompositionProcessorEngine = _pCompositionProcessorEngine;

    // pCompositionProcessorEngine->ToggleIMEMode(_GetThreadMgr(), _GetClientId());

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionInput
//
// If the keystroke happens within a composition, eat the key and return S_OK.
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionInput(TfEditCookie ec, _In_ ITfContext *pContext, WCHAR wch)
{
    ITfRange *pRangeComposition = nullptr;
    TF_SELECTION tfSelection;
    ULONG fetched = 0;
    BOOL isCovered = TRUE;

    CCompositionProcessorEngine *pCompositionProcessorEngine = nullptr;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;

    if ((_pCandidateListUIPresenter != nullptr) && (_candidateMode != CANDIDATE_INCREMENTAL))
    {
        _HandleCompositionFinalize(ec, pContext, FALSE);
    }

    // Start the new (std::nothrow) compositon if there is no composition.
    if (!_IsComposing())
    {
        _StartComposition(pContext);
    }

    // first, test where a keystroke would go in the document if we did an insert
    if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched) != S_OK || fetched != 1)
    {
        return S_FALSE;
    }

    // is the insertion point covered by a composition?
    if (SUCCEEDED(_pComposition->GetRange(&pRangeComposition)))
    {
        isCovered = _IsRangeCovered(ec, tfSelection.range, pRangeComposition);

        pRangeComposition->Release();

        if (!isCovered)
        {
            goto Exit;
        }
    }

#ifdef FANY_DEBUG
    OutputDebugString(L"Fany AddVirtualKey Here.");
#endif
    // Add virtual key to composition processor engine
    pCompositionProcessorEngine->AddVirtualKey(wch);

    _HandleCompositionInputWorker(pCompositionProcessorEngine, ec, pContext);

Exit:
    tfSelection.range->Release();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionInputWorker
//
// If the keystroke happens within a composition, eat the key and return S_OK.
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionInputWorker(_In_ CCompositionProcessorEngine *pCompositionProcessorEngine,
                                                       TfEditCookie ec, _In_ ITfContext *pContext)
{
    HRESULT hr = S_OK;
    CMetasequoiaImeArray<CStringRange> readingStrings;
    BOOL isWildcardIncluded = FALSE;

    //
    // Get reading string from composition processor engine
    //
    pCompositionProcessorEngine->GetReadingStrings(&readingStrings, &isWildcardIncluded);

    if (readingStrings.Count())
    {
#ifdef FANY_DEBUG
        // TODO: Log reading strings
#endif
    }

    for (UINT index = 0; index < readingStrings.Count(); index++)
    {
        hr = _AddComposingAndChar(ec, pContext, readingStrings.GetAt(index));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // Get candidate string from composition processor engine
    //
    CMetasequoiaImeArray<CCandidateListItem> candidateList;

    //
    // Important: Generate candidate list here
    //
    // There is no need to use neither IncrementalWordSearch nor WildcardSearch, so we set them both FALSE
    pCompositionProcessorEngine->GetCandidateList(&candidateList, FALSE, FALSE);

    if ((candidateList.Count()))
    {
        hr = _CreateAndStartCandidate(pCompositionProcessorEngine, ec, pContext);
        if (SUCCEEDED(hr))
        {
            _pCandidateListUIPresenter->_ClearList();
            _pCandidateListUIPresenter->_SetText(&candidateList, TRUE);
        }
    }
    else if (_pCandidateListUIPresenter)
    {
        _pCandidateListUIPresenter->_ClearList();
    }
    else if (readingStrings.Count() && isWildcardIncluded)
    {
        hr = _CreateAndStartCandidate(pCompositionProcessorEngine, ec, pContext);
        if (SUCCEEDED(hr))
        {
            _pCandidateListUIPresenter->_ClearList();
        }
    }
    return hr;
}
//+---------------------------------------------------------------------------
//
// _CreateAndStartCandidate
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_CreateAndStartCandidate(_In_ CCompositionProcessorEngine *pCompositionProcessorEngine,
                                                  TfEditCookie ec, _In_ ITfContext *pContext)
{
    HRESULT hr = S_OK;

    if (((_candidateMode == CANDIDATE_PHRASE) && (_pCandidateListUIPresenter)) ||
        ((_candidateMode == CANDIDATE_NONE) && (_pCandidateListUIPresenter)))
    {
        // Recreate candidate list
        _pCandidateListUIPresenter->_EndCandidateList();
        delete _pCandidateListUIPresenter;
        _pCandidateListUIPresenter = nullptr;

        _candidateMode = CANDIDATE_NONE;
        _isCandidateWithWildcard = FALSE;
    }

    if (_pCandidateListUIPresenter == nullptr)
    {
        _pCandidateListUIPresenter = new (std::nothrow)
            CCandidateListUIPresenter(this, Global::AtomCandidateWindow, CATEGORY_CANDIDATE,
                                      pCompositionProcessorEngine->GetCandidateListIndexRange(), FALSE);
        if (!_pCandidateListUIPresenter)
        {
            return E_OUTOFMEMORY;
        }

        _candidateMode = CANDIDATE_INCREMENTAL;
        _isCandidateWithWildcard = FALSE;

        // we don't cache the document manager object. So get it from pContext.
        ITfDocumentMgr *pDocumentMgr = nullptr;
        if (SUCCEEDED(pContext->GetDocumentMgr(&pDocumentMgr)))
        {
            // get the composition range.
            ITfRange *pRange = nullptr;
            if (SUCCEEDED(_pComposition->GetRange(&pRange)))
            {
                hr = _pCandidateListUIPresenter->_StartCandidateList(
                    _tfClientId, pDocumentMgr, pContext, ec, pRange,
                    pCompositionProcessorEngine->GetCandidateWindowWidth());
                pRange->Release();
            }
            pDocumentMgr->Release();
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionFinalize
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionFinalize(TfEditCookie ec, _In_ ITfContext *pContext, BOOL isCandidateList)
{
    HRESULT hr = S_OK;

    if (isCandidateList && _pCandidateListUIPresenter)
    {
        // Finalize selected candidate string from CCandidateListUIPresenter
        DWORD_PTR candidateLen = 0;
        const WCHAR *pCandidateString = nullptr;

        candidateLen = _pCandidateListUIPresenter->_GetSelectedCandidateString(&pCandidateString);

        CStringRange candidateString;
        candidateString.Set(pCandidateString, candidateLen);

        if (candidateLen)
        {
            // Finalize character
            hr = _AddCharAndFinalize(ec, pContext, &candidateString);
            if (FAILED(hr))
            {
                return hr;
            }
        }
    }
    else
    {
        // Finalize current text store strings
        if (_IsComposing())
        {
            ULONG fetched = 0;
            TF_SELECTION tfSelection;

            if (FAILED(pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) || fetched != 1)
            {
                return S_FALSE;
            }

            ITfRange *pRangeComposition = nullptr;
            if (SUCCEEDED(_pComposition->GetRange(&pRangeComposition)))
            {
                if (_IsRangeCovered(ec, tfSelection.range, pRangeComposition))
                {
                    _EndComposition(pContext);
                }

                pRangeComposition->Release();
            }

            tfSelection.range->Release();
        }
    }

    _HandleCancel(ec, pContext);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionConvert
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionConvert(TfEditCookie ec, _In_ ITfContext *pContext, BOOL isWildcardSearch)
{
    HRESULT hr = S_OK;

    CMetasequoiaImeArray<CCandidateListItem> candidateList;

    //
    // Get candidate string from composition processor engine
    //
    CCompositionProcessorEngine *pCompositionProcessorEngine = nullptr;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;
    pCompositionProcessorEngine->GetCandidateList(&candidateList, FALSE, isWildcardSearch);

    // If there is no candlidate listin the current reading string, we don't do anything. Just wait for
    // next char to be ready for the conversion with it.
    int nCount = candidateList.Count();
    if (nCount)
    {
        if (_pCandidateListUIPresenter)
        {
            _pCandidateListUIPresenter->_EndCandidateList();
            delete _pCandidateListUIPresenter;
            _pCandidateListUIPresenter = nullptr;

            _candidateMode = CANDIDATE_NONE;
            _isCandidateWithWildcard = FALSE;
        }

        //
        // create an instance of the candidate list class.
        //
        if (_pCandidateListUIPresenter == nullptr)
        {
            _pCandidateListUIPresenter = new (std::nothrow)
                CCandidateListUIPresenter(this, Global::AtomCandidateWindow, CATEGORY_CANDIDATE,
                                          pCompositionProcessorEngine->GetCandidateListIndexRange(), FALSE);
            if (!_pCandidateListUIPresenter)
            {
                return E_OUTOFMEMORY;
            }

            _candidateMode = CANDIDATE_ORIGINAL;
        }

        _isCandidateWithWildcard = isWildcardSearch;

        // we don't cache the document manager object. So get it from pContext.
        ITfDocumentMgr *pDocumentMgr = nullptr;
        if (SUCCEEDED(pContext->GetDocumentMgr(&pDocumentMgr)))
        {
            // get the composition range.
            ITfRange *pRange = nullptr;
            if (SUCCEEDED(_pComposition->GetRange(&pRange)))
            {
                hr = _pCandidateListUIPresenter->_StartCandidateList(
                    _tfClientId, pDocumentMgr, pContext, ec, pRange,
                    pCompositionProcessorEngine->GetCandidateWindowWidth());
                pRange->Release();
            }
            pDocumentMgr->Release();
        }
        if (SUCCEEDED(hr))
        {
            _pCandidateListUIPresenter->_SetText(&candidateList, FALSE);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionBackspace
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionBackspace(TfEditCookie ec, _In_ ITfContext *pContext)
{
    ITfRange *pRangeComposition = nullptr;
    TF_SELECTION tfSelection;
    ULONG fetched = 0;
    BOOL isCovered = TRUE;

    // Start the new (std::nothrow) compositon if there is no composition.
    if (!_IsComposing())
    {
        return S_OK;
    }

    // first, test where a keystroke would go in the document if we did an insert
    if (FAILED(pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) || fetched != 1)
    {
        return S_FALSE;
    }

    // is the insertion point covered by a composition?
    if (SUCCEEDED(_pComposition->GetRange(&pRangeComposition)))
    {
        isCovered = _IsRangeCovered(ec, tfSelection.range, pRangeComposition);

        pRangeComposition->Release();

        if (!isCovered)
        {
            goto Exit;
        }
    }

    //
    // Add virtual key to composition processor engine
    //
    CCompositionProcessorEngine *pCompositionProcessorEngine = nullptr;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;

    DWORD_PTR vKeyLen = pCompositionProcessorEngine->GetVirtualKeyLength();

    if (vKeyLen)
    {
        pCompositionProcessorEngine->RemoveVirtualKey(vKeyLen - 1);

        if (pCompositionProcessorEngine->GetVirtualKeyLength())
        {
            _HandleCompositionInputWorker(pCompositionProcessorEngine, ec, pContext);
        }
        else
        {
            _HandleCancel(ec, pContext);
        }
    }

Exit:
    tfSelection.range->Release();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionArrowKey
//
// Update the selection within a composition.
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionArrowKey(TfEditCookie ec, _In_ ITfContext *pContext,
                                                    KEYSTROKE_FUNCTION keyFunction)
{
    ITfRange *pRangeComposition = nullptr;
    TF_SELECTION tfSelection;
    ULONG fetched = 0;

    // get the selection
    if (FAILED(pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) || fetched != 1)
    {
        // no selection, eat the keystroke
        return S_OK;
    }

    // get the composition range
    if (FAILED(_pComposition->GetRange(&pRangeComposition)))
    {
        goto Exit;
    }

    // For incremental candidate list
    if (_pCandidateListUIPresenter)
    {
        _pCandidateListUIPresenter->AdviseUIChangedByArrowKey(keyFunction);
    }

    pContext->SetSelection(ec, 1, &tfSelection);

    pRangeComposition->Release();

Exit:
    tfSelection.range->Release();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionPunctuation
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionPunctuation(TfEditCookie ec, _In_ ITfContext *pContext, WCHAR wch)
{
    HRESULT hr = S_OK;
    //
    // Get punctuation char from composition processor engine
    //
    CCompositionProcessorEngine *pCompositionProcessorEngine = nullptr;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;

    const WCHAR *punctuation = pCompositionProcessorEngine->GetPunctuation(wch);
    std::wstring punctuationStr(punctuation, wcslen(punctuation));

    if (_candidateMode != CANDIDATE_NONE && _pCandidateListUIPresenter)
    {
        //
        // Request for first candidate string for some
        //
        if (Global::CommitWithFirstCandPunc.count(wch) > 0)
        {
            /* Here we do not need to consider index out of range, cause we always retrive first candidate */
            struct FanyImeNamedpipeDataToTsf *receivedData = TryReadDataFromServerPipeWithTimeout();
            punctuationStr = std::wstring(receivedData->candidate_string) + punctuationStr;
        }
    }

    CStringRange punctuationString;
    punctuationString.Set(punctuationStr.c_str(), punctuationStr.length());

    /* Finalize character */
    _RemoveDummyCompositionForComposing(ec, _pComposition); // Clear dummy original pinyin composition
    hr = _AddCharAndFinalize(ec, pContext, &punctuationString);
    if (FAILED(hr))
    {
        return hr;
    }

    _HandleComplete(ec, pContext);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleCompositionDoubleSingleByte
//
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_HandleCompositionDoubleSingleByte(TfEditCookie ec, _In_ ITfContext *pContext, WCHAR wch)
{
    HRESULT hr = S_OK;

    WCHAR fullWidth = Global::FullWidthCharTable[wch - 0x20];

    CStringRange fullWidthString;
    fullWidthString.Set(&fullWidth, 1);

    // Finalize character
    hr = _AddCharAndFinalize(ec, pContext, &fullWidthString);
    if (FAILED(hr))
    {
        return hr;
    }

    _HandleCancel(ec, pContext);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InvokeKeyHandler
//
// This text service is interested in handling keystrokes to demonstrate the
// use the compositions. Some apps will cancel compositions if they receive
// keystrokes while a compositions is ongoing.
//
// param
//    [in] uCode - virtual key code of WM_KEYDOWN wParam
//    [in] dwFlags - WM_KEYDOWN lParam
//    [in] dwKeyFunction - Function regarding virtual key
//----------------------------------------------------------------------------

HRESULT CMetasequoiaIME::_InvokeKeyHandler(_In_ ITfContext *pContext, UINT code, WCHAR wch, DWORD flags,
                                           _KEYSTROKE_STATE keyState)
{
    flags;

    CKeyHandlerEditSession *pEditSession = nullptr;
    HRESULT hr = E_FAIL;

    // we'll insert a char ourselves in place of this keystroke
    pEditSession = new (std::nothrow) CKeyHandlerEditSession(this, pContext, code, wch, keyState);
    if (pEditSession == nullptr)
    {
        goto Exit;
    }

    //
    // Call CKeyHandlerEditSession::DoEditSession().
    //
    // Do not specify TF_ES_SYNC so edit session is not invoked on WinWord
    //
    hr = pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);

    pEditSession->Release();

Exit:
    return hr;
}
