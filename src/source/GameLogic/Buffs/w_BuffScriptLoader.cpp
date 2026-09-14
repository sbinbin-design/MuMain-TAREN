// BuffScriptLoader.cpp: implementation of the CBuffScriptLoader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Core/Utilities/ReadScript.h"
#include "UI/Legacy/UIManager.h"
#include "GameLogic/Items/ItemAddOptioninfo.h"
#include "w_BuffScriptLoader.h"
#include "Core/Platform/WinCompat.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Data/GameConfig/GameConfig.h"

#include <cwctype>
#include <set>
#include <utility>
#include <vector>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace
{
constexpr WORD kLocalizedBuffChecksumKey = 0xE2F1;
constexpr DWORD kLocalizedBuffRecordSize = sizeof(_BUFFINFO);

void CutTokenString(wchar_t* pcCuttoken, std::list<std::wstring>& out)
{
    if (wcslen(pcCuttoken) == 0)
        return;

    int cutpos = 0;

    for (int i = 0; i < MAX_DESCRIPT_LENGTH; ++i)
    {
        if (pcCuttoken[i] == '/' || pcCuttoken[i] == 0)
        {
            wchar_t Temp[MAX_DESCRIPT_LENGTH] = {
                0,
            };
            wcsncpy(Temp, pcCuttoken + cutpos, i - cutpos);
            out.push_back(Temp);
            cutpos = i + 1;

            if (pcCuttoken[i] == 0)
                return;
        }
    }
}

bool HasIllegalControlCharacters(const std::wstring& text)
{
    for (const wchar_t character : text)
    {
        if (character < 0x20 && character != L'\t' && character != L'\n' && character != L'\r')
            return true;
    }

    return false;
}

bool GetPrintfSignature(const std::wstring& text, std::vector<wchar_t>& signature)
{
    for (size_t index = 0; index < text.size(); ++index)
    {
        if (text[index] != L'%')
            continue;

        if (index + 1 < text.size() && text[index + 1] == L'%')
        {
            signature.push_back(L'%');
            ++index;
            continue;
        }

        size_t specifier = index + 1;
        while (specifier < text.size() && wcschr(L"-+ #0'", text[specifier]) != nullptr)
            ++specifier;
        while (specifier < text.size() && iswdigit(text[specifier]) != 0)
            ++specifier;
        if (specifier < text.size() && text[specifier] == L'*')
            ++specifier;
        if (specifier < text.size() && text[specifier] == L'.')
        {
            ++specifier;
            if (specifier < text.size() && text[specifier] == L'*')
                ++specifier;
            else
            {
                while (specifier < text.size() && iswdigit(text[specifier]) != 0)
                    ++specifier;
            }
        }
        while (specifier < text.size() && wcschr(L"hljztL", text[specifier]) != nullptr)
            ++specifier;

        if (specifier >= text.size() || wcschr(L"diuoxXfFeEgGaAcspn", text[specifier]) == nullptr)
            return false;

        signature.push_back(text[specifier]);
        index = specifier;
    }

    return true;
}

bool IsSafeBusinessMatch(const BuffInfo& original, const _BUFFINFO& official)
{
    const bool effectMatches = original.s_BuffEffectType == official.s_BuffEffectType;
    const bool itemTypeMatches = original.s_ItemType == official.s_ItemType;
    const bool itemIndexMatches = original.s_ItemIndex == official.s_ItemIndex;
    const bool classMatches = original.s_BuffClassType == official.s_BuffClassType;
    const bool noticeMatches = original.s_NoticeType == official.s_NoticeType;
    const bool clearMatches = original.s_ClearType == official.s_ClearType;

    if (itemTypeMatches && itemIndexMatches && classMatches && noticeMatches && clearMatches && effectMatches)
        return true;

    const short index = original.s_BuffIndex;
    if (index == 29 || index == 30 || index == 31 || index == 44 || index == 45 || index == 46 || index == 47
        || index == 48 || index == 49 || index == 89 || index == 90 || index == 121)
    {
        return itemTypeMatches && itemIndexMatches && classMatches && noticeMatches && clearMatches;
    }

    if (index == 40 || index == 41 || index == 42 || index == 43)
    {
        return effectMatches && itemTypeMatches && itemIndexMatches && classMatches && clearMatches;
    }

    return false;
}

bool IsFixedEnglishFallback(short index)
{
    return index == 87 || index == 88 || index == 106;
}

bool DecodeLocalizedField(std::wstring& target, const char* source, size_t capacity)
{
    const size_t length = strnlen(source, capacity);
    if (length == 0)
        return false;

    std::vector<wchar_t> decoded(length + 1, L'\0');
    if (CMultiLanguage::ConvertFromMuChineseLegacy(decoded.data(), source, static_cast<int>(length)) <= 0)
        return false;

    target.assign(decoded.data());
    return !target.empty() && !HasIllegalControlCharacters(target);
}

bool IsDescriptionCompatible(const std::wstring& english, const std::wstring& chinese)
{
    std::vector<wchar_t> englishSignature;
    std::vector<wchar_t> chineseSignature;
    return GetPrintfSignature(english, englishSignature) && GetPrintfSignature(chinese, chineseSignature)
        && englishSignature == chineseSignature;
}
} // namespace

BuffInfo::BuffInfo()
    : s_BuffIndex(0), s_BuffEffectType(0), s_ItemType(255), s_ItemIndex(255), s_BuffClassType(0), s_NoticeType(0),
      s_ClearType(0)
{
    memset(&s_BuffName, 0, sizeof(wchar_t) * MAX_BUFF_NAME_LENGTH);
    memset(&s_BuffDescript, 0, sizeof(wchar_t) * MAX_DESCRIPT_LENGTH);
}

BuffInfo::~BuffInfo() {}

BuffScriptLoaderPtr BuffScriptLoader::Make()
{
    BuffScriptLoaderPtr info(new BuffScriptLoader());
    return info;
}

BuffScriptLoader::BuffScriptLoader()
{
    std::wstring filename = L"data/local/" + g_strSelectedML + L"/BuffEffect_" + g_strSelectedML + L".bmd";

    if (!Load(filename))
    {
        assert(0);
    }

    if (GameConfig::GetInstance().IsSimplifiedChineseLocale())
        LoadLocalizedText();
}

BuffScriptLoader::~BuffScriptLoader() {}

bool BuffScriptLoader::Load(const std::wstring& pchFileName)
{
    FILE* fp = _wfopen(pchFileName.c_str(), L"rb");

    if (fp != NULL)
    {
        DWORD structsize = sizeof(_BUFFINFO);

        DWORD listsize;
        fread(&listsize, sizeof(DWORD), 1, fp);

        BYTE* Buffer = new BYTE[structsize * listsize];
        fread(Buffer, structsize * listsize, 1, fp);

        DWORD dwCheckSum;
        fread(&dwCheckSum, sizeof(DWORD), 1, fp);

        fclose(fp);
        if (dwCheckSum != GenerateCheckSum2(Buffer, structsize * listsize, 0xE2F1))
        {
            mu::log::Get("gameplay")->warn("{} - File corrupted.", mu_wchar_to_utf8(pchFileName.c_str()));
            wchar_t Text[256];
            mu_swprintf(Text, L"%ls - File corrupted.", pchFileName.c_str());
            MessageBox(g_hWnd, Text, NULL, MB_OK);
            SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        }
        else
        {
            BYTE* pSeek = Buffer;

            for (DWORD i = 0; i < listsize; i++)
            {
                _BUFFINFO tempbuffinfo;

                BuxConvert(pSeek, structsize);
                memcpy(&tempbuffinfo, pSeek, structsize);

                BuffInfo buffinfo;
                buffinfo.s_BuffIndex = tempbuffinfo.s_BuffIndex;
                buffinfo.s_BuffEffectType = tempbuffinfo.s_BuffEffectType;
                buffinfo.s_ItemType = tempbuffinfo.s_ItemType;
                buffinfo.s_ItemIndex = tempbuffinfo.s_ItemIndex;

                CMultiLanguage::ConvertFromUtf8(buffinfo.s_BuffName, tempbuffinfo.s_BuffName, MAX_BUFF_NAME_LENGTH);
                CMultiLanguage::ConvertFromUtf8(buffinfo.s_BuffDescript, tempbuffinfo.s_BuffDescript,
                                                MAX_DESCRIPT_LENGTH);

                buffinfo.s_BuffClassType = tempbuffinfo.s_BuffClassType;
                buffinfo.s_NoticeType = tempbuffinfo.s_NoticeType;
                buffinfo.s_ClearType = tempbuffinfo.s_ClearType;

                CutTokenString(buffinfo.s_BuffDescript, buffinfo.s_BuffDescriptlist);
                m_Info.insert(std::make_pair(static_cast<eBuffState>(buffinfo.s_BuffIndex), buffinfo));

                pSeek += structsize;
            }
        }
        delete[] Buffer;
    }
    else
    {
        mu::log::Get("gameplay")->warn("{} - File not exist.", mu_wchar_to_utf8(pchFileName.c_str()));
        wchar_t Text[256];
        mu_swprintf(Text, L"%ls - File not exist.", pchFileName.c_str());
        MessageBox(g_hWnd, Text, NULL, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
    }

    return true;
}

const BuffInfo BuffScriptLoader::GetBuffinfo(eBuffState type) const
{
    if (type >= eBuff_Count)
        return BuffInfo();

    auto iter = m_Info.find(type);

    if (iter != m_Info.end())
    {
        BuffInfo result = (*iter).second;
        const auto localized = m_LocalizedText.find(type);
        if (localized != m_LocalizedText.end())
        {
            if (!localized->second.Name.empty())
            {
                wcsncpy(result.s_BuffName, localized->second.Name.c_str(), MAX_BUFF_NAME_LENGTH - 1);
                result.s_BuffName[MAX_BUFF_NAME_LENGTH - 1] = L'\0';
            }
            if (!localized->second.Description.empty())
            {
                wcsncpy(result.s_BuffDescript, localized->second.Description.c_str(), MAX_DESCRIPT_LENGTH - 1);
                result.s_BuffDescript[MAX_DESCRIPT_LENGTH - 1] = L'\0';
                result.s_BuffDescriptlist.clear();
                CutTokenString(result.s_BuffDescript, result.s_BuffDescriptlist);
            }
        }

        return result;
    }

    return BuffInfo();
}

bool BuffScriptLoader::LoadLocalizedText()
{
    FILE* file = _wfopen(L"Data\\Local\\BuffEffect.bmd", L"rb");
    if (file == nullptr)
        return false;

    DWORD recordCount = 0;
    if (fread(&recordCount, sizeof(recordCount), 1, file) != 1 || recordCount == 0 || recordCount > eBuff_Count)
    {
        fclose(file);
        return false;
    }

    const size_t dataSize = static_cast<size_t>(recordCount) * kLocalizedBuffRecordSize;
    std::vector<BYTE> buffer(dataSize);
    if (fread(buffer.data(), 1, dataSize, file) != dataSize)
    {
        fclose(file);
        return false;
    }

    DWORD checksum = 0;
    if (fread(&checksum, sizeof(checksum), 1, file) != 1)
    {
        fclose(file);
        return false;
    }
    if (fgetc(file) != EOF)
    {
        fclose(file);
        return false;
    }
    fclose(file);

    if (checksum != GenerateCheckSum2(buffer.data(), static_cast<DWORD>(dataSize), kLocalizedBuffChecksumKey))
        return false;

    std::set<short> seenIndices;
    std::map<eBuffState, LocalizedBuffText> localizedText;
    for (DWORD record = 0; record < recordCount; ++record)
    {
        std::vector<BYTE> decrypted(buffer.begin() + record * kLocalizedBuffRecordSize,
                                    buffer.begin() + (record + 1) * kLocalizedBuffRecordSize);
        BuxConvert(decrypted.data(), static_cast<int>(decrypted.size()));

        _BUFFINFO official{};
        memcpy(&official, decrypted.data(), sizeof(official));
        const auto original = m_Info.find(static_cast<eBuffState>(official.s_BuffIndex));
        if (original == m_Info.end() || IsFixedEnglishFallback(official.s_BuffIndex)
            || !IsSafeBusinessMatch(original->second, official))
            continue;
        if (!seenIndices.insert(official.s_BuffIndex).second)
            return false;

        LocalizedBuffText localized;
        if (!DecodeLocalizedField(localized.Name, official.s_BuffName, MAX_BUFF_NAME_LENGTH))
            continue;

        std::wstring description;
        if (DecodeLocalizedField(description, official.s_BuffDescript, MAX_DESCRIPT_LENGTH)
            && IsDescriptionCompatible(std::wstring(original->second.s_BuffDescript), description))
        {
            localized.Description = std::move(description);
        }
        localizedText[static_cast<eBuffState>(official.s_BuffIndex)] = std::move(localized);
    }

    m_LocalizedText.swap(localizedText);
    return true;
}

eBuffClass BuffScriptLoader::IsBuffClass(eBuffState type) const
{
    if (type >= eBuff_Count)
        return eBuffClass_Count;

    auto iter = m_Info.find(type);

    if (iter != m_Info.end())
    {
        return static_cast<eBuffClass>((*iter).second.s_BuffClassType);
    }
    else
    {
        return eBuffClass_Count;
    }
}

#ifdef KJH_PBG_ADD_INGAMESHOP_SYSTEM
int BuffScriptLoader::GetBuffIndex(int iItemCode)
{
    auto iter = m_Info.begin();

    int iterItemCode = 0;

    while (iter != m_Info.end())
    {
        iterItemCode = ITEMINDEX(iter->second.s_ItemType, iter->second.s_ItemIndex);

        if (iterItemCode == iItemCode)
            return iter->second.s_BuffIndex;

        iter++;
    }

    return -1;
}

int BuffScriptLoader::GetBuffType(int iItemCode)
{
    auto iter = m_Info.begin();

    int iterItemCode = 0;

    while (iter != m_Info.end())
    {
        iterItemCode = ITEMINDEX(iter->second.s_ItemType, iter->second.s_ItemIndex);

        if (iterItemCode == iItemCode)
            return iter->second.s_BuffEffectType;

        iter++;
    }

    return -1;
}
#endif // KJH_PBG_ADD_INGAMESHOP_SYSTEM
