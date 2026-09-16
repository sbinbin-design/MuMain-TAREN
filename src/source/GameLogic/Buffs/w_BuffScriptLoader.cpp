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

#include <set>
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

bool IsOfficialOnlyBuff(short index)
{
    return (index >= 122 && index <= 128) || (index >= 170 && index <= 173) || index == 185;
}

bool IsSupportedBuffIdentity(short index)
{
    return index > eBuffNone && index < eBuff_Count && !IsOfficialOnlyBuff(index);
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

void ApplyMuMainCompatibility(_BUFFINFO& buff)
{
    switch (buff.s_BuffIndex)
    {
    case 29:
    case 30:
    case 31:
        buff.s_BuffEffectType = 24;
        break;
    case 40:
    case 41:
    case 42:
    case 43:
        buff.s_NoticeType = 0;
        break;
    case 44:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
        buff.s_BuffEffectType = static_cast<BYTE>(buff.s_BuffIndex + 23);
        break;
    case 87:
    case 88:
        buff.s_BuffEffectType = 13;
        break;
    case 89:
    case 90:
        buff.s_BuffEffectType = 14;
        break;
    case 106:
        buff.s_ItemType = 255;
        buff.s_ItemIndex = 255;
        break;
    case 121:
        buff.s_BuffEffectType = 75;
        break;
    default:
        break;
    }
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
    const bool isSimplifiedChinese = GameConfig::GetInstance().IsSimplifiedChineseLocale();
    if (isSimplifiedChinese)
    {
        constexpr wchar_t kLocalizedBuffFile[] = L"Data\\Local\\BuffEffect.bmd";
        if (!LoadOfficialLocalizedBuffEffect())
        {
            mu::log::Get("gameplay")->warn("{} - File corrupted or unavailable.",
                                            mu_wchar_to_utf8(kLocalizedBuffFile));
            wchar_t Text[256];
            mu_swprintf(Text, L"%ls - File corrupted or unavailable.", kLocalizedBuffFile);
            MessageBox(g_hWnd, Text, NULL, MB_OK);
            SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        }
        return;
    }

    const std::wstring filename = L"data/local/" + g_strSelectedML + L"/BuffEffect_" + g_strSelectedML + L".bmd";
    if (!Load(filename))
        assert(0);
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
        return (*iter).second;
    }

    return BuffInfo();
}

bool BuffScriptLoader::LoadOfficialLocalizedBuffEffect()
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
    BuffInfoMap officialInfo;
    for (DWORD record = 0; record < recordCount; ++record)
    {
        std::vector<BYTE> decrypted(buffer.begin() + record * kLocalizedBuffRecordSize,
                                    buffer.begin() + (record + 1) * kLocalizedBuffRecordSize);
        BuxConvert(decrypted.data(), static_cast<int>(decrypted.size()));

        _BUFFINFO official{};
        memcpy(&official, decrypted.data(), sizeof(official));
        if (!IsSupportedBuffIdentity(official.s_BuffIndex))
            continue;
        if (!seenIndices.insert(official.s_BuffIndex).second)
            return false;

        ApplyMuMainCompatibility(official);

        BuffInfo buffinfo;
        buffinfo.s_BuffIndex = official.s_BuffIndex;
        buffinfo.s_BuffEffectType = official.s_BuffEffectType;
        buffinfo.s_ItemType = official.s_ItemType;
        buffinfo.s_ItemIndex = official.s_ItemIndex;
        buffinfo.s_BuffClassType = official.s_BuffClassType;
        buffinfo.s_NoticeType = official.s_NoticeType;
        buffinfo.s_ClearType = official.s_ClearType;

        std::wstring name;
        if (!DecodeLocalizedField(name, official.s_BuffName, MAX_BUFF_NAME_LENGTH))
            return false;

        wcsncpy(buffinfo.s_BuffName, name.c_str(), MAX_BUFF_NAME_LENGTH - 1);
        buffinfo.s_BuffName[MAX_BUFF_NAME_LENGTH - 1] = L'\0';

        if (official.s_BuffDescript[0] != '\0')
        {
            std::wstring description;
            if (!DecodeLocalizedField(description, official.s_BuffDescript, MAX_DESCRIPT_LENGTH))
                return false;
            wcsncpy(buffinfo.s_BuffDescript, description.c_str(), MAX_DESCRIPT_LENGTH - 1);
            buffinfo.s_BuffDescript[MAX_DESCRIPT_LENGTH - 1] = L'\0';
        }

        CutTokenString(buffinfo.s_BuffDescript, buffinfo.s_BuffDescriptlist);
        officialInfo.insert(std::make_pair(static_cast<eBuffState>(buffinfo.s_BuffIndex), buffinfo));
    }

    m_Info.swap(officialInfo);
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
