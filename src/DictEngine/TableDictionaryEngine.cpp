#include "Private.h"
#include "TableDictionaryEngine.h"
#include "DictionarySearch.h"
#include "Globals.h"
#include <algorithm>
#include <string>
#include <vector>
#include "FanyUtils.h"

#ifdef FANY_DEBUG
#include <chrono>
#endif

//+---------------------------------------------------------------------------
//
// CollectWord
//
//----------------------------------------------------------------------------

VOID CTableDictionaryEngine::CollectWord(_In_ CStringRange *pKeyCode,
                                         _Inout_ CMetasequoiaImeArray<CStringRange> *pWordStrings)
{
    CDictionaryResult *pdret = nullptr;
    CDictionarySearch dshSearch(_locale, _pDictionaryFile, pKeyCode);

    while (dshSearch.FindPhrase(&pdret))
    {
        for (UINT index = 0; index < pdret->_FindPhraseList.Count(); index++)
        {
            CStringRange *pPhrase = nullptr;
            pPhrase = pWordStrings->Append();
            if (pPhrase)
            {
                *pPhrase = *pdret->_FindPhraseList.GetAt(index);
            }
        }

        delete pdret;
        pdret = nullptr;
    }
}

VOID CTableDictionaryEngine::CollectWord(_In_ CStringRange *pKeyCode,
                                         _Inout_ CMetasequoiaImeArray<CCandidateListItem> *pItemList)
{
    std::wstring keyCodeWString = L"";
    keyCodeWString.append(pKeyCode->Get(), pKeyCode->GetLength()); // Append the key code to the string
    std::string keyCodeString = FanyUtils::wstring_to_string(keyCodeWString);
    // Convert the key code string to lower case
    std::transform(keyCodeString.begin(), keyCodeString.end(), keyCodeString.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    Global::WStringCandidateList.clear();
    Global::FindKeyCode = keyCodeWString;
    Global::WStringCandidateList.push_back(Global::FindKeyCode);

    for (UINT i = 0; i < Global::WStringCandidateList.size(); i++)
    {
        const std::wstring &wstr = Global::WStringCandidateList[i];

        CCandidateListItem *pLI = nullptr;
        pLI = pItemList->Append();
        if (pLI)
        {
            pLI->_ItemString.Set(wstr.c_str(), wstr.size());
            pLI->_FindKeyCode.Set(Global::FindKeyCode.c_str(), Global::FindKeyCode.size());
        }
    }

    if (!pItemList->Count())
    {
        CCandidateListItem *pLI = nullptr;
        pLI = pItemList->Append();
        pLI->_ItemString.Set(Global::FindKeyCode.c_str(), Global::FindKeyCode.size());
        pLI->_FindKeyCode.Set(Global::FindKeyCode.c_str(), Global::FindKeyCode.size());
    }
}

//+---------------------------------------------------------------------------
//
// CollectWordForWildcard
//
//----------------------------------------------------------------------------

VOID CTableDictionaryEngine::CollectWordForWildcard(_In_ CStringRange *pKeyCode,
                                                    _Inout_ CMetasequoiaImeArray<CCandidateListItem> *pItemList)
{
    CDictionaryResult *pdret = nullptr;
    CDictionarySearch dshSearch(_locale, _pDictionaryFile, pKeyCode);

    while (dshSearch.FindPhraseForWildcard(&pdret))
    {
        for (UINT iIndex = 0; iIndex < pdret->_FindPhraseList.Count(); iIndex++)
        {
            CCandidateListItem *pLI = nullptr;
            pLI = pItemList->Append();
            if (pLI)
            {
                pLI->_ItemString.Set(*pdret->_FindPhraseList.GetAt(iIndex));
                pLI->_FindKeyCode.Set(pdret->_FindKeyCode.Get(), pdret->_FindKeyCode.GetLength());
            }
        }

        delete pdret;
        pdret = nullptr;
    }

    if (!pItemList->Count())
    {
        CCandidateListItem *pLI = nullptr;
        pLI = pItemList->Append();
        // 使用用户输入的关键字作为默认值
        const WCHAR *userInput = pKeyCode->Get();          // 获取用户输入的关键字
        DWORD_PTR userInputLength = pKeyCode->GetLength(); // 获取用户输入的长度
        pLI->_ItemString.Set(userInput, userInputLength);  // 设置用户输入为默认值
        pLI->_FindKeyCode.Set(userInput, userInputLength);
    }
}

//+---------------------------------------------------------------------------
//
// CollectWordFromConvertedStringForWildcard
//
//----------------------------------------------------------------------------

VOID CTableDictionaryEngine::CollectWordFromConvertedStringForWildcard(
    _In_ CStringRange *pString, _Inout_ CMetasequoiaImeArray<CCandidateListItem> *pItemList)
{
    CDictionaryResult *pdret = nullptr;
    CDictionarySearch dshSearch(_locale, _pDictionaryFile, pString);

    while (dshSearch.FindConvertedStringForWildcard(&pdret)) // TAIL ALL CHAR MATCH
    {
        for (UINT index = 0; index < pdret->_FindPhraseList.Count(); index++)
        {
            CCandidateListItem *pLI = nullptr;
            pLI = pItemList->Append();
            if (pLI)
            {
                pLI->_ItemString.Set(*pdret->_FindPhraseList.GetAt(index));
                pLI->_FindKeyCode.Set(pdret->_FindKeyCode.Get(), pdret->_FindKeyCode.GetLength());
            }
        }

        delete pdret;
        pdret = nullptr;
    }

    if (!pItemList->Count())
    {
        CCandidateListItem *pLI = nullptr;
        pLI = pItemList->Append();
        // 使用用户输入的关键字作为默认值
        const WCHAR *userInput = pString->Get();          // 获取用户输入的关键字
        DWORD_PTR userInputLength = pString->GetLength(); // 获取用户输入的长度
        pLI->_ItemString.Set(userInput, userInputLength); // 设置用户输入为默认值
        pLI->_FindKeyCode.Set(userInput, userInputLength);
    }
}
