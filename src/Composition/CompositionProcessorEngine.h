#pragma once

#include "MetasequoiaIME.h"
#include "sal.h"
#include "TableDictionaryEngine.h"
#include "KeyHandlerEditSession.h"
#include "MetasequoiaIMEBaseStructure.h"
#include "FileMapping.h"
#include "Compartment.h"
#include "define.h"

class CCompositionProcessorEngine
{
    friend class CMetasequoiaIME;

  public:
    CCompositionProcessorEngine(void);
    ~CCompositionProcessorEngine(void);

    BOOL SetupLanguageProfile(LANGID langid, REFGUID guidLanguageProfile, _In_ ITfThreadMgr *pThreadMgr,
                              TfClientId tfClientId, BOOL isSecureMode, BOOL isComLessMode);

    // Get language profile.
    GUID GetLanguageProfile(LANGID *plangid)
    {
        *plangid = _langid;
        return _guidProfile;
    }
    // Get locale
    LCID GetLocale()
    {
        return MAKELCID(_langid, SORT_DEFAULT);
    }

    BOOL IsVirtualKeyNeed(UINT uCode, _In_reads_(1) WCHAR *pwch, BOOL fComposing, CANDIDATE_MODE candidateMode,
                          BOOL hasCandidateWithWildcard, _Out_opt_ _KEYSTROKE_STATE *pKeyState);

    BOOL AddVirtualKey(WCHAR wch);
    void RemoveVirtualKey(DWORD_PTR dwIndex);
    void PurgeVirtualKey();

    DWORD_PTR GetVirtualKeyLength()
    {
        return _keystrokeBuffer.GetLength();
    }
    CStringRange &GetKeystrokeBuffer()
    {
        return _keystrokeBuffer;
    };
    WCHAR GetVirtualKey(DWORD_PTR dwIndex);

    void GetReadingStrings(                                          //
        _Inout_ CMetasequoiaImeArray<CStringRange> *pReadingStrings, //
        _Out_ BOOL *pIsWildcardIncluded                              //
    );
    void GetCandidateList(                                                //
        _Inout_ CMetasequoiaImeArray<CCandidateListItem> *pCandidateList, //
        BOOL isIncrementalWordSearch, BOOL isWildcardSearch               //
    );
    void GetCandidateStringInConverted(CStringRange &searchString,
                                       _In_ CMetasequoiaImeArray<CCandidateListItem> *pCandidateList);

    // Preserved key handler
    void OnPreservedKey(ITfContext *pContext, REFGUID rguid, _Out_ BOOL *pIsEaten, _In_ ITfThreadMgr *pThreadMgr,
                        TfClientId tfClientId, BOOL *pNeedToggleIMEMode);

    // Toggle IME Mode
    void ToggleIMEMode(_In_ ITfThreadMgr *pThreadMgr, TfClientId tfClientId);

    // Punctuation
    BOOL IsPunctuation(WCHAR wch);
    const WCHAR *GetPunctuation(WCHAR wch);

    BOOL IsDoubleSingleByte(WCHAR wch);
    BOOL IsWildcard()
    {
        return _isWildcard;
    }
    BOOL IsDisableWildcardAtFirst()
    {
        return _isDisableWildcardAtFirst;
    }
    BOOL IsWildcardChar(WCHAR wch)
    {
        return ((IsWildcardOneChar(wch) || IsWildcardAllChar(wch)) ? TRUE : FALSE);
    }
    BOOL IsWildcardOneChar(WCHAR wch)
    {
        return (wch == L'?' ? TRUE : FALSE);
    }
    BOOL IsWildcardAllChar(WCHAR wch)
    {
        return (wch == L'*' ? TRUE : FALSE);
    }
    BOOL IsMakePhraseFromText()
    {
        // return _hasMakePhraseFromText;
        return 0;
    }
    BOOL IsKeystrokeSort()
    {
        return _isKeystrokeSort;
    }

    // Dictionary engine
    BOOL IsDictionaryAvailable()
    {
        return (_pTableDictionaryEngine ? TRUE : FALSE);
    }

    // Language bar control
    void SetLanguageBarStatus(DWORD status, BOOL isSet);

    void ConversionModeCompartmentUpdated(_In_ ITfThreadMgr *pThreadMgr);

    void ShowAllLanguageBarIcons();
    void HideAllLanguageBarIcons();

    inline CCandidateRange *GetCandidateListIndexRange()
    {
        return &_candidateListIndexRange;
    }
    inline UINT GetCandidateListPhraseModifier()
    {
        return _candidateListPhraseModifier;
    }
    inline UINT GetCandidateWindowWidth()
    {
        return _candidateWndWidth;
    }

  private:
    void InitKeyStrokeTable();
    BOOL InitLanguageBar(_In_ CLangBarItemButton *pLanguageBar, _In_ ITfThreadMgr *pThreadMgr, TfClientId tfClientId,
                         REFGUID guidCompartment);

    struct _KEYSTROKE;
    BOOL IsVirtualKeyKeystrokeComposition(UINT uCode, _Out_opt_ _KEYSTROKE_STATE *pKeyState,
                                          KEYSTROKE_FUNCTION function);
    BOOL IsVirtualKeyKeystrokeCandidate(UINT uCode, _In_ _KEYSTROKE_STATE *pKeyState, CANDIDATE_MODE candidateMode,
                                        _Out_ BOOL *pfRetCode, _In_ CMetasequoiaImeArray<_KEYSTROKE> *pKeystrokeMetric);
    BOOL IsKeystrokeRange(UINT uCode, _Out_ _KEYSTROKE_STATE *pKeyState, CANDIDATE_MODE candidateMode);

    void SetupKeystroke();
    void SetupPreserved(_In_ ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    void SetupConfiguration();
    void SetupLanguageBar(_In_ ITfThreadMgr *pThreadMgr, TfClientId tfClientId, BOOL isSecureMode);
    void SetKeystrokeTable(_Inout_ CMetasequoiaImeArray<_KEYSTROKE> *pKeystroke);
    void SetupPunctuationPair();
    void CreateLanguageBarButton(DWORD dwEnable, GUID guidLangBar, _In_z_ LPCWSTR pwszDescriptionValue,
                                 _In_z_ LPCWSTR pwszTooltipValue, DWORD dwOnIconIndex, DWORD dwOffIconIndex,
                                 _Outptr_result_maybenull_ CLangBarItemButton **ppLangBarItemButton, BOOL isSecureMode);
    void SetInitialCandidateListRange();
    void SetDefaultCandidateTextFont();
    void InitializeMetasequoiaIMECompartment(_In_ ITfThreadMgr *pThreadMgr, TfClientId tfClientId);

    class XPreservedKey;
    void SetPreservedKey(const CLSID clsid, TF_PRESERVEDKEY &tfPreservedKey, _In_z_ LPCWSTR pwszDescription,
                         _Out_ XPreservedKey *pXPreservedKey);
    BOOL InitPreservedKey(_In_ XPreservedKey *pXPreservedKey, _In_ ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    BOOL CheckShiftKeyOnly(_In_ CMetasequoiaImeArray<TF_PRESERVEDKEY> *pTSFPreservedKeyTable);

    static HRESULT CompartmentCallback(_In_ void *pv, REFGUID guidCompartment);
    void PrivateCompartmentsUpdated(_In_ ITfThreadMgr *pThreadMgr);
    void KeyboardOpenCompartmentUpdated(_In_ ITfThreadMgr *pThreadMgr);

    BOOL SetupDictionaryFile();
    CFile *GetDictionaryFile();

  private:
    struct _KEYSTROKE
    {
        UINT VirtualKey;
        UINT Modifiers;
        KEYSTROKE_FUNCTION Function;

        _KEYSTROKE()
        {
            VirtualKey = 0;
            Modifiers = 0;
            Function = FUNCTION_NONE;
        }
    };
    _KEYSTROKE _keystrokeTable[26];

    CTableDictionaryEngine *_pTableDictionaryEngine;
    CStringRange _keystrokeBuffer;

    BOOL _hasWildcardIncludedInKeystrokeBuffer;

    LANGID _langid;
    GUID _guidProfile;
    TfClientId _tfClientId;

    CMetasequoiaImeArray<_KEYSTROKE> _KeystrokeComposition;
    CMetasequoiaImeArray<_KEYSTROKE> _KeystrokeCandidate;
    CMetasequoiaImeArray<_KEYSTROKE> _KeystrokeCandidateWildcard;
    CMetasequoiaImeArray<_KEYSTROKE> _KeystrokeCandidateSymbol;
    CMetasequoiaImeArray<_KEYSTROKE> _KeystrokeSymbol;

    // Preserved key data
    class XPreservedKey
    {
      public:
        XPreservedKey();
        ~XPreservedKey();
        BOOL UninitPreservedKey(_In_ ITfThreadMgr *pThreadMgr);

      public:
        CMetasequoiaImeArray<TF_PRESERVEDKEY> TSFPreservedKeyTable;
        GUID Guid;
        LPCWSTR Description;
    };

    XPreservedKey _PreservedKey_IMEMode;
    XPreservedKey _PreservedKey_IMEMode02;
    XPreservedKey _PreservedKey_DoubleSingleByte;
    XPreservedKey _PreservedKey_Punctuation;

    // Punctuation data
    CMetasequoiaImeArray<CPunctuationPair> _PunctuationPair;
    CMetasequoiaImeArray<CPunctuationNestPair> _PunctuationNestPair;

    // Language bar data
    CLangBarItemButton *_pLanguageBar_IMEMode;
    CLangBarItemButton *_pLanguageBar_DoubleSingleByte;
    CLangBarItemButton *_pLanguageBar_Punctuation;

    // Compartment
    CCompartment *_pCompartmentConversion;
    CCompartmentEventSink *_pCompartmentConversionEventSink;
    CCompartmentEventSink *_pCompartmentKeyboardOpenEventSink;
    CCompartmentEventSink *_pCompartmentDoubleSingleByteEventSink;
    CCompartmentEventSink *_pCompartmentPunctuationEventSink;

    // Configuration data
    BOOL _isWildcard : 1;
    BOOL _isDisableWildcardAtFirst : 1;
    BOOL _hasMakePhraseFromText : 1;
    BOOL _isKeystrokeSort : 1;
    BOOL _isComLessMode : 1;
    CCandidateRange _candidateListIndexRange;
    UINT _candidateListPhraseModifier;
    UINT _candidateWndWidth;

    CFileMapping *_pDictionaryFile;

    static const int OUT_OF_FILE_INDEX = -1;
};
