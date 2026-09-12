#include "stdafx.h"
#include "App/Platform/Windows/Winmain.h"
#include "Render/Textures/ZzzTexture.h"
#include "GameLogic/Items/CSItemOption.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/HUD/NewUIMasterLevel.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "GameLogic/Skills/SkillManager.h"
#include "Data/DataHandler/SkillData/SkillDataHandler.h"
#include "Data/GameConfig/GameConfig.h"

#include <cstddef>
#include <cwchar>
#include <iterator>
#include <utility>

namespace 
{
    _MASTER_SKILLTREE_DATA m_stMasterSkillTreeData[MAX_MASTER_SKILL_DATA];
    _MASTER_SKILL_TOOLTIP m_stMasterSkillTooltip[MAX_MASTER_SKILL_DATA];

    constexpr std::size_t kMasterSkillTooltipPayloadSize =
        sizeof(_MASTER_SKILL_TOOLTIP_FILE) * MAX_MASTER_SKILL_DATA;
    constexpr unsigned int kMasterSkillTooltipCodePage = 54936u;
    constexpr MASTER_SKILL_TREE_CLASS kLocalizedClassBits[] =
    {
        MASTER_SKILL_TREE_CLASS_BLADEMASTER,
        MASTER_SKILL_TREE_CLASS_GRANDMASTER,
        MASTER_SKILL_TREE_CLASS_HIGHELF,
        MASTER_SKILL_TREE_CLASS_DIMENSIONMASTER,
        MASTER_SKILL_TREE_CLASS_DUELMASTER,
        MASTER_SKILL_TREE_CLASS_LORDEMPEROR,
        MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT,
    };

    struct LocalizedTooltipException
    {
        int skill;
        MASTER_SKILL_TREE_CLASS activeClass;
    };

    constexpr LocalizedTooltipException kLocalizedTooltipExceptions[] =
    {
        { 363, MASTER_SKILL_TREE_CLASS_BLADEMASTER },
        { 426, MASTER_SKILL_TREE_CLASS_HIGHELF },
        { 460, MASTER_SKILL_TREE_CLASS_DIMENSIONMASTER },
        { 554, MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT },
        { 555, MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT },
        { 565, MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT },
        { 593, MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT },
    };

    static_assert(sizeof(_MASTER_SKILL_TOOLTIP_FILE) == 616);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info1) == 6);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info2) == 70);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info3) == 326);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info4) == 358);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info5) == 422);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info6) == 486);
    static_assert(offsetof(_MASTER_SKILL_TOOLTIP_FILE, Info7) == 550);

    bool IsLocalizedTooltipException(ActionSkillType skill, MASTER_SKILL_TREE_CLASS activeClass)
    {
        for (const auto& exception : kLocalizedTooltipExceptions)
        {
            if (exception.skill == skill && exception.activeClass == activeClass)
                return true;
        }
        return false;
    }

    bool IsPrintfConversion(wchar_t value)
    {
        return value != L'\0' && wcschr(L"diouxXfFeEgGaAcsp%", value) != nullptr;
    }

    bool GetPrintfToken(const wchar_t* format, std::size_t& position, std::wstring& token)
    {
        const std::size_t start = position++;
        if (format[position] == L'%')
        {
            ++position;
            token.assign(format + start, position - start);
            return true;
        }

        while (format[position] != L'\0' && wcschr(L"-+ #0'", format[position]) != nullptr)
            ++position;
        while (format[position] >= L'0' && format[position] <= L'9')
            ++position;
        if (format[position] == L'*' || format[position] == L'$')
            return false;
        if (format[position] == L'.')
        {
            ++position;
            if (format[position] == L'*')
                return false;
            while (format[position] >= L'0' && format[position] <= L'9')
                ++position;
        }
        while (format[position] != L'\0' && wcschr(L"hlLzjt", format[position]) != nullptr)
            ++position;
        if (!IsPrintfConversion(format[position]) || format[position] == L'%')
            return false;

        ++position;
        token.assign(format + start, position - start);
        return true;
    }

    bool GetPrintfSignature(const wchar_t* format, std::vector<std::wstring>& signature)
    {
        signature.clear();
        for (std::size_t position = 0; format[position] != L'\0';)
        {
            if (format[position] != L'%')
            {
                ++position;
                continue;
            }

            std::wstring token;
            if (!GetPrintfToken(format, position, token))
                return false;
            signature.push_back(std::move(token));
        }
        return true;
    }

    bool HasCompatiblePrintfSignature(const wchar_t* english, const wchar_t* localized)
    {
        std::vector<std::wstring> englishSignature;
        std::vector<std::wstring> localizedSignature;
        return GetPrintfSignature(english, englishSignature) &&
            GetPrintfSignature(localized, localizedSignature) && englishSignature == localizedSignature;
    }

    bool DecodeFixedCodePageField(const char* source, std::size_t sourceCapacity,
                                  wchar_t* target, std::size_t targetCapacity, unsigned int codePage)
    {
        std::size_t sourceLength = 0;
        while (sourceLength < sourceCapacity && source[sourceLength] != '\0')
            ++sourceLength;
        std::wstring converted;
        if (!CMultiLanguage::ConvertFromCodePageToString(
                converted, source, codePage, static_cast<int>(sourceLength)) ||
            converted.size() >= targetCapacity)
        {
            return false;
        }

        wmemcpy(target, converted.c_str(), converted.size());
        target[converted.size()] = L'\0';
        return true;
    }

    bool DecodeLocalizedTooltip(const _MASTER_SKILL_TOOLTIP_FILE& source,
                                _LOCALIZED_MASTER_SKILL_TOOLTIP& target)
    {
        return DecodeFixedCodePageField(source.Info1, sizeof(source.Info1), target.Info1, std::size(target.Info1),
                                        kMasterSkillTooltipCodePage) &&
            DecodeFixedCodePageField(source.Info2, sizeof(source.Info2), target.Info2, std::size(target.Info2),
                                     kMasterSkillTooltipCodePage) &&
            DecodeFixedCodePageField(source.Info3, sizeof(source.Info3), target.Info3, std::size(target.Info3),
                                     kMasterSkillTooltipCodePage) &&
            DecodeFixedCodePageField(source.Info4, sizeof(source.Info4), target.Info4, std::size(target.Info4),
                                     kMasterSkillTooltipCodePage) &&
            DecodeFixedCodePageField(source.Info5, sizeof(source.Info5), target.Info5, std::size(target.Info5),
                                     kMasterSkillTooltipCodePage) &&
            DecodeFixedCodePageField(source.Info6, sizeof(source.Info6), target.Info6, std::size(target.Info6),
                                     kMasterSkillTooltipCodePage) &&
            DecodeFixedCodePageField(source.Info7, sizeof(source.Info7), target.Info7, std::size(target.Info7),
                                     kMasterSkillTooltipCodePage);
    }

    bool HasCompatibleLocalizedTooltip(const _MASTER_SKILL_TOOLTIP& english,
                                       const _LOCALIZED_MASTER_SKILL_TOOLTIP& localized)
    {
        return HasCompatiblePrintfSignature(english.Info1, localized.Info1) &&
            HasCompatiblePrintfSignature(english.Info2, localized.Info2) &&
            HasCompatiblePrintfSignature(english.Info3, localized.Info3) &&
            HasCompatiblePrintfSignature(english.Info4, localized.Info4) &&
            HasCompatiblePrintfSignature(english.Info5, localized.Info5) &&
            HasCompatiblePrintfSignature(english.Info6, localized.Info6) &&
            HasCompatiblePrintfSignature(english.Info7, localized.Info7);
    }
}


SEASON3B::CNewUIMasterLevel::CNewUIMasterLevel()
{
    m_pNewUIMng = nullptr;
    this->ConsumePoint = 0;
    this->CurSkillID = 0;
    this->classCode = MASTER_SKILL_TREE_CLASS_NONE;
    this->CategoryTextIndex = 0;
    this->categoryPos[0] = { 11,55 };
    this->categoryPos[1] = { 221,55 };
    this->categoryPos[2] = { 431,55 };
    this->InitMasterSkillPoint();
    this->ClearSkillTreeData();
    this->ClearSkillTooltipData();
    this->ClearLocalizedSkillTooltipData();
}

SEASON3B::CNewUIMasterLevel::~CNewUIMasterLevel()
{
    this->Release();
}

BYTE SEASON3B::CNewUIMasterLevel::GetConsumePoint() const
{
    return this->ConsumePoint;
}

int SEASON3B::CNewUIMasterLevel::GetCurSkillID() const
{
    return this->CurSkillID;
}

bool SEASON3B::CNewUIMasterLevel::Create(CNewUIManager* pNewUIMng)
{
    if (nullptr == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MASTER_LEVEL, this);

    this->SetPos();

    this->LoadImages();

    this->m_CloseBT.ChangeButtonImgState(true, IMAGE_MASTER_INTERFACE + 5, false, false, false);

    this->m_CloseBT.ChangeButtonInfo(611, 9, 13, 14);

    this->m_CloseBT.ChangeToolTipText(&I18N::Game::Close388);

    for (int i = 0; i < MAX_MASTER_SKILL_CATEGORY; i++)
    {
        this->ButtonX[i] = 0;

        this->ButtonY[i] = 0;
    }

    return true;
}

void SEASON3B::CNewUIMasterLevel::Release()
{
    this->ClearSkillTreeData();
    this->ClearSkillTooltipData();
    this->ClearLocalizedSkillTooltipData();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIMasterLevel::SetPos()
{
    this->PosX = 0;
    this->PosY = 0;
    this->width = REFERENCE_WIDTH;
    this->height = 428;
}

void SEASON3B::CNewUIMasterLevel::OpenMasterSkillTreeData(const wchar_t* path)
{
    memset(m_stMasterSkillTreeData, 0, sizeof(m_stMasterSkillTreeData));

    FILE* fp = _wfopen(path, L"rb");

    wchar_t Text[256];

    if (fp == nullptr)
    {
        mu_swprintf(Text, L"%ls - File not exist.", path);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }

    constexpr int Size = sizeof(_MASTER_SKILLTREE_DATA);

    auto Buffer = new BYTE[Size * MAX_MASTER_SKILL_DATA];

    fread(Buffer, Size * MAX_MASTER_SKILL_DATA, 1, fp);

    DWORD dwCheckSum;

    fread(&dwCheckSum, sizeof(DWORD), 1u, fp);

    fclose(fp);

    if (dwCheckSum != GenerateCheckSum2(Buffer, 12288, 0x2BC1))
    {
        mu_swprintf(Text, L"%ls - File corrupted.", path);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }

    BYTE* pSeek = Buffer;

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        BuxConvert(pSeek, Size);

        memcpy(&m_stMasterSkillTreeData[i], pSeek, Size);

        pSeek += Size;

        if (pSeek == nullptr)
        {
            break;
        }
    }

    delete[] Buffer;
}

void SEASON3B::CNewUIMasterLevel::OpenMasterSkillTooltip(const wchar_t* path)
{
    memset(m_stMasterSkillTooltip, 0, sizeof(m_stMasterSkillTooltip));

    FILE* fp = _wfopen(path, L"rb");

    if (fp == nullptr)
    {
        wchar_t Text[256];
        mu_swprintf(Text, L"%ls - File not exist.", path);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }

    constexpr int record_size = sizeof(_MASTER_SKILL_TOOLTIP_FILE);
    auto file_buffer = new BYTE[record_size * MAX_MASTER_SKILL_DATA];
    fread(file_buffer, record_size * MAX_MASTER_SKILL_DATA, 1, fp);
    DWORD dwCheckSum;
    fread(&dwCheckSum, sizeof(DWORD), 1u, fp);
    fclose(fp);

    BYTE* pSeek = file_buffer;

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        BuxConvert(pSeek, record_size);

        _MASTER_SKILL_TOOLTIP_FILE current{ };
        memcpy(&current, pSeek, record_size);

        const auto target = &m_stMasterSkillTooltip[i];
        target->SkillNumber = static_cast<ActionSkillType>(current.SkillNumber);
        target->ClassCode = static_cast<MASTER_SKILL_TREE_CLASS>(current.ClassCode);
        DecodeFixedCodePageField(current.Info1, sizeof(current.Info1), target->Info1, std::size(target->Info1), CP_UTF8);
        DecodeFixedCodePageField(current.Info2, sizeof(current.Info2), target->Info2, std::size(target->Info2), CP_UTF8);
        DecodeFixedCodePageField(current.Info3, sizeof(current.Info3), target->Info3, std::size(target->Info3), CP_UTF8);
        DecodeFixedCodePageField(current.Info4, sizeof(current.Info4), target->Info4, std::size(target->Info4), CP_UTF8);
        DecodeFixedCodePageField(current.Info5, sizeof(current.Info5), target->Info5, std::size(target->Info5), CP_UTF8);
        DecodeFixedCodePageField(current.Info6, sizeof(current.Info6), target->Info6, std::size(target->Info6), CP_UTF8);
        DecodeFixedCodePageField(current.Info7, sizeof(current.Info7), target->Info7, std::size(target->Info7), CP_UTF8);

        pSeek += record_size;

        if (pSeek == nullptr)
        {
            break;
        }
    }

    delete[] file_buffer;

}

void SEASON3B::CNewUIMasterLevel::LoadLocalizedMasterSkillTooltip(const wchar_t* path)
{
    this->map_localizedMasterSkillToolTipByClass.clear();
    this->map_localizedMasterSkillToolTip.clear();

    FILE* fp = _wfopen(path, L"rb");
    if (fp == nullptr)
        return;

    fseek(fp, 0, SEEK_END);
    const long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fileSize != static_cast<long>(kMasterSkillTooltipPayloadSize + sizeof(DWORD)))
    {
        fclose(fp);
        return;
    }

    std::vector<BYTE> buffer(kMasterSkillTooltipPayloadSize);
    DWORD checksum = 0;
    const bool readPayload = fread(buffer.data(), kMasterSkillTooltipPayloadSize, 1, fp) == 1;
    const bool readChecksum = fread(&checksum, sizeof(checksum), 1, fp) == 1;
    fclose(fp);
    if (!readPayload || !readChecksum ||
        GenerateCheckSum2(buffer.data(), static_cast<DWORD>(kMasterSkillTooltipPayloadSize), 0x2BC1) != checksum)
    {
        return;
    }

    using LocalizedTooltipKey = std::pair<ActionSkillType, MASTER_SKILL_TREE_CLASS>;
    std::set<LocalizedTooltipKey> muMainKeys;
    for (const auto& masterSkillTooltip : m_stMasterSkillTooltip)
    {
        if (masterSkillTooltip.SkillNumber < AT_SKILL_MASTER_BEGIN ||
            masterSkillTooltip.SkillNumber > AT_SKILL_MASTER_END)
        {
            continue;
        }

        for (const auto activeClass : kLocalizedClassBits)
        {
            if ((masterSkillTooltip.ClassCode & activeClass) != 0)
                muMainKeys.emplace(masterSkillTooltip.SkillNumber, activeClass);
        }
    }

    std::map<LocalizedTooltipKey, _LOCALIZED_MASTER_SKILL_TOOLTIP> loaded;
    std::set<LocalizedTooltipKey> invalidKeys;
    for (int i = 0; i < MAX_MASTER_SKILL_DATA; ++i)
    {
        auto* record = buffer.data() + i * sizeof(_MASTER_SKILL_TOOLTIP_FILE);
        BuxConvert(record, sizeof(_MASTER_SKILL_TOOLTIP_FILE));

        _MASTER_SKILL_TOOLTIP_FILE source{};
        memcpy(&source, record, sizeof(source));
        if (source.SkillNumber < AT_SKILL_MASTER_BEGIN || source.SkillNumber > AT_SKILL_MASTER_END)
            continue;

        bool hasEligibleKey = false;
        for (const auto activeClass : kLocalizedClassBits)
        {
            const auto key = std::make_pair(static_cast<ActionSkillType>(source.SkillNumber), activeClass);
            if ((source.ClassCode & activeClass) != 0 && muMainKeys.contains(key) &&
                !IsLocalizedTooltipException(static_cast<ActionSkillType>(source.SkillNumber), activeClass))
            {
                hasEligibleKey = true;
                break;
            }
        }
        if (!hasEligibleKey)
            continue;

        _LOCALIZED_MASTER_SKILL_TOOLTIP localized{};
        if (!DecodeLocalizedTooltip(source, localized))
            continue;

        for (const auto activeClass : kLocalizedClassBits)
        {
            if ((source.ClassCode & activeClass) == 0)
                continue;

            const auto key = std::make_pair(static_cast<ActionSkillType>(source.SkillNumber), activeClass);
            if (muMainKeys.find(key) == muMainKeys.end() || IsLocalizedTooltipException(
                    static_cast<ActionSkillType>(source.SkillNumber), activeClass))
            {
                continue;
            }
            if (invalidKeys.find(key) != invalidKeys.end())
                continue;
            if (!loaded.emplace(key, localized).second)
            {
                loaded.erase(key);
                invalidKeys.insert(key);
            }
        }
    }

    this->map_localizedMasterSkillToolTipByClass.swap(loaded);
}

void SEASON3B::CNewUIMasterLevel::InitMasterSkillPoint()
{
    for (int i = 0; i < 3; i++)
    {
        this->CategoryPoint[i] = 0;
        for (int k = 0; k < 10; k++)
        {
            this->skillPoint[i][k] = 0;
        }
    }
}

void SEASON3B::CNewUIMasterLevel::SetMasterType(CLASS_TYPE Class)
{
    switch (Class)
    {
    case CLASS_GRANDMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_GRANDMASTER;
        break;
    case CLASS_BLADEMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_BLADEMASTER;
        break;
    case CLASS_HIGHELF:
        this->classCode = MASTER_SKILL_TREE_CLASS_HIGHELF;
        break;
    case CLASS_DUELMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_DUELMASTER;
        break;
    case CLASS_LORDEMPEROR:
        this->classCode = MASTER_SKILL_TREE_CLASS_LORDEMPEROR;
        break;
    case CLASS_DIMENSIONMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_DIMENSIONMASTER;
        break;
    case CLASS_TEMPLENIGHT:
        this->classCode = MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT;
        break;
    default:
        break;
    }

    this->SetMasterSkillTreeData();

    this->SetMasterSkillToolTipData();

    if (GameConfig::GetInstance().GetUILocale() == L"zh-CN")
    {
        this->LoadLocalizedMasterSkillTooltip(L"Data\\Local\\MasterSkillTooltip.bmd");
        this->SetLocalizedMasterSkillToolTipData();
    }

    switch (Class)
    {
    case CLASS_WIZARD:
    case CLASS_SOULMASTER:
    case CLASS_GRANDMASTER:
        this->CategoryTextIndex = 1751;
        this->ClassNameTextIndex = 1669;
        break;
    case CLASS_KNIGHT:
    case CLASS_BLADEKNIGHT:
    case CLASS_BLADEMASTER:
        this->CategoryTextIndex = 1755;
        this->ClassNameTextIndex = 1668;
        break;
    case CLASS_ELF:
    case CLASS_MUSEELF:
    case CLASS_HIGHELF:
        this->CategoryTextIndex = 1759;
        this->ClassNameTextIndex = 1670;
        break;
    case CLASS_DARK:
    case CLASS_DUELMASTER:
        this->CategoryTextIndex = 1763;
        this->ClassNameTextIndex = 1671;
        break;
    case CLASS_DARK_LORD:
    case CLASS_LORDEMPEROR:
        this->CategoryTextIndex = 1767;
        this->ClassNameTextIndex = 1672;
        break;
    case CLASS_SUMMONER:
    case CLASS_BLOODYSUMMONER:
    case CLASS_DIMENSIONMASTER:
        this->CategoryTextIndex = 3136;
        this->ClassNameTextIndex = 1689;
        break;
    case CLASS_RAGEFIGHTER:
    case CLASS_TEMPLENIGHT:
        this->CategoryTextIndex = 3330;
        this->ClassNameTextIndex = 3151;
        break;
    default:
        return;
    }
}

void SEASON3B::CNewUIMasterLevel::SetMasterSkillTreeData()
{
    this->ClearSkillTreeData();

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        if (m_stMasterSkillTreeData[i].Index == 0)
        {
            break;
        }

        if ((this->classCode & m_stMasterSkillTreeData[i].ClassCode) == 0)
        {
            continue;
        }

        if (!this->map_masterData.insert(std::pair<BYTE, _MASTER_SKILLTREE_DATA>(m_stMasterSkillTreeData[i].Index, m_stMasterSkillTreeData[i])).second)
        {
            break;
        }
    }
}

void SEASON3B::CNewUIMasterLevel::SetMasterSkillToolTipData()
{
    this->ClearSkillTooltipData();

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        if (m_stMasterSkillTooltip[i].SkillNumber == 0)
        {
            break;
        }

        if ((this->classCode & m_stMasterSkillTooltip[i].ClassCode) == 0)
        {
            continue;
        }

        if (!this->map_masterSkillToolTip.insert(std::pair(m_stMasterSkillTooltip[i].SkillNumber, m_stMasterSkillTooltip[i])).second)
        {
            break;
        }
    }

}

void SEASON3B::CNewUIMasterLevel::SetLocalizedMasterSkillToolTipData()
{
    this->map_localizedMasterSkillToolTip.clear();
    if (GameConfig::GetInstance().GetUILocale() != L"zh-CN")
        return;

    for (const auto& [skill, english] : this->map_masterSkillToolTip)
    {
        const auto localized = this->map_localizedMasterSkillToolTipByClass.find(
            std::make_pair(skill, this->classCode));
        if (localized == this->map_localizedMasterSkillToolTipByClass.end() ||
            !HasCompatibleLocalizedTooltip(english, localized->second))
        {
            continue;
        }

        this->map_localizedMasterSkillToolTip.emplace(skill, localized->second);
    }
}

bool SEASON3B::CNewUIMasterLevel::SetMasterSkillTreeInfo(int index, BYTE skillLevel, float value, float nextvalue)
{
    const auto it = this->map_masterData.find(index);

    if (it == this->map_masterData.end())
    {
        return false;
    }

    const CSkillTreeInfo skillInfo = { skillLevel, value, nextvalue };
    CharacterAttribute->MasterSkillInfo[it->second.Skill] = skillInfo;

    this->CategoryPoint[it->second.Group] += skillLevel;

    return true;
}

int SEASON3B::CNewUIMasterLevel::SetDivideString(wchar_t* text, int isItemTollTip, int TextNum, int iTextColor, int iTextBold, bool isPercent)
{
    if (text == nullptr)
    {
        return TextNum;
    }

    constexpr wchar_t alpszDst[10][256] = {};

    int  nLine = 0;

    if (isItemTollTip == 0)
    {
        nLine = DivideStringByPixel((LPTSTR)alpszDst, 10, 256, text, 150, true, 35);
    }
    else if (isItemTollTip == 1)
    {
        nLine = DivideStringByPixel((LPTSTR)alpszDst, 10, 256, text, 200, true, 35);
    }

    for (int i = 0; i < nLine; i++)
    {
        TextListColor[TextNum] = iTextColor;

        TextBold[TextNum] = iTextBold;

        std::wstring cText = alpszDst[i];

        if (isPercent)
        {
            for (int j = cText.find(L"%", 0); j != -1; j = cText.find(L"%", j + 2))
            {
                cText.insert(j, L"%");
            }
        }

        mu_swprintf(TextList[TextNum], cText.c_str());

        TextNum++;
    }

    return TextNum;
}

bool SEASON3B::CNewUIMasterLevel::Render()
{
    EnableAlphaTest();
    RenderImage(IMAGE_MASTER_INTERFACE, this->PosX, this->PosY, Bitmaps[IMAGE_MASTER_INTERFACE].Width, Bitmaps[IMAGE_MASTER_INTERFACE].Height);
    RenderImage(IMAGE_MASTER_INTERFACE + 1, this->PosX + Bitmaps[IMAGE_MASTER_INTERFACE].Width, this->PosY, Bitmaps[IMAGE_MASTER_INTERFACE + 1].Width, Bitmaps[IMAGE_MASTER_INTERFACE + 1].Height);
    this->RenderIcon();
    this->m_CloseBT.Render();
    DisableAlphaBlend();
    this->RenderText();

    return true;
}

bool SEASON3B::CNewUIMasterLevel::Update()
{
    return true;
}

bool SEASON3B::CNewUIMasterLevel::UpdateMouseEvent()
{
    if (this->m_CloseBT.UpdateMouseEvent() == true)
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_MASTER_LEVEL);

        return true;
    }

    bool result = true;

    if (SEASON3B::IsPress(VK_LBUTTON) == true)
    {
        result = this->CheckMouse(MouseX, MouseY);

        if (result == false)
        {
            PlayBuffer(SOUND_CLICK01);
        }
    }

    for (int i = 0; i < MAX_MASTER_SKILL_CATEGORY; i++)
    {
        if (this->ButtonX[i] == 1 && SEASON3B::IsPress(VK_LBUTTON) == true)
        {
            this->ButtonX[i] = 0;

            return true;
        }
    }

    this->CheckBtn();

    if (SEASON3B::CheckMouseIn(this->PosX, this->PosY, this->width, this->height) == true)
    {
        return false;
    }

    return result;
}

bool SEASON3B::CNewUIMasterLevel::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MASTER_LEVEL) == false || SEASON3B::IsPress(VK_ESCAPE) == false && SEASON3B::IsPress('A') == false)
    {
        return true;
    }

    g_pNewUISystem->Hide(SEASON3B::INTERFACE_MASTER_LEVEL);

    PlayBuffer(SOUND_CLICK01);

    return false;
}

float SEASON3B::CNewUIMasterLevel::GetLayerDepth()
{
    return 10.1000004;
}

void SEASON3B::CNewUIMasterLevel::LoadImages()
{
    LoadBitmap(L"Interface\\new_Master_back01.jpg", IMAGE_MASTER_INTERFACE, GL_LINEAR, GL_REPEAT, true, false);
    LoadBitmap(L"Interface\\new_Master_back02.jpg", IMAGE_MASTER_INTERFACE + 1, GL_LINEAR, GL_REPEAT, true, false);
    LoadBitmap(L"Interface\\new_Master_Icon.jpg", IMAGE_MASTER_INTERFACE + 2, GL_LINEAR, GL_CLAMP, true, false);
    LoadBitmap(L"Interface\\new_Master_Non_Icon.jpg", IMAGE_MASTER_INTERFACE + 3, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_box.tga", IMAGE_MASTER_INTERFACE + 4, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_exit.jpg", IMAGE_MASTER_INTERFACE + 5, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow01.tga", IMAGE_MASTER_INTERFACE + 6, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow02.tga", IMAGE_MASTER_INTERFACE + 7, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow03.tga", IMAGE_MASTER_INTERFACE + 8, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow04.tga", IMAGE_MASTER_INTERFACE + 9, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow05.tga", IMAGE_MASTER_INTERFACE + 10, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow06.tga", IMAGE_MASTER_INTERFACE + 11, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow07.tga", IMAGE_MASTER_INTERFACE + 12, GL_LINEAR);
    LoadBitmap(L"Interface\\new_Master_arrow08.tga", IMAGE_MASTER_INTERFACE + 13, GL_LINEAR);
}

void SEASON3B::CNewUIMasterLevel::UnloadImages()
{
    for (int i = 0; i < 14; i++)
    {
        DeleteBitmap(i + IMAGE_MASTER_INTERFACE, false);
    }
}

void SEASON3B::CNewUIMasterLevel::RenderText() const
{
    g_pRenderText->SetFont(g_hFont);

    if (SEASON3B::IsPress(VK_LBUTTON) == false && SEASON3B::CheckMouseIn(458, 11, 81, 10) == true)
    {
        TextList[0][0] = 0;
        TextBold[0] = 0;
        TextListColor[0] = 0;
        mu_swprintf(TextList[0], L"%I64d / %I64d", Master_Level_Data.lMasterLevel_Experince, Master_Level_Data.lNext_MasterLevel_Experince);
        RenderTipTextList(466, 26, 1, 0, 3, 0, 1);
    }

    g_pRenderText->SetTextColor(255, 255, 255, 0xFFu);

    g_pRenderText->SetBgColor(0, 0, 0, 1u);

    wchar_t Buffer[256] = {};

    mu_swprintf(Buffer, I18N::Game::MasterLevelD, Master_Level_Data.nMLevel);

    g_pRenderText->RenderText(275, 11, Buffer, 0, 0, 1, 0);

    mu_swprintf(Buffer, I18N::Game::LevelPointD, Master_Level_Data.nMLevelUpMPoint);

    g_pRenderText->RenderText(372, 11, Buffer, 0, 0, 1, 0);

    if (Master_Level_Data.lNext_MasterLevel_Experince != 0)
    {
        const __int64 iTotalLevel = Master_Level_Data.nMLevel + 400;				// 종합레벨 - 400렙이 만렙이기 때문에 더해준다.
        const __int64 iTOverLevel = iTotalLevel - 255;		// 255레벨 이상 기준 레벨
        __int64 iBaseExperience = 0;					// 레벨 초기 경험치

        const __int64 iData_Master =	// A
            (
                (
                    (__int64)9 + (__int64)iTotalLevel
                    )
                * (__int64)iTotalLevel
                * (__int64)iTotalLevel
                * (__int64)10
                )
            +
            (
                (
                    (__int64)9 + (__int64)iTOverLevel
                    )
                * (__int64)iTOverLevel
                * (__int64)iTOverLevel
                * (__int64)1000
                );

        iBaseExperience = (iData_Master - (__int64)3892250000) / (__int64)2;	// B

        // 레벨업 경험치
        const double fNeedExp = (double)Master_Level_Data.lNext_MasterLevel_Experince - (double)iBaseExperience;

        // 현재 획득한 경험치
        const double fExp = (double)Master_Level_Data.lMasterLevel_Experince - (double)iBaseExperience;

        mu_swprintf(Buffer, I18N::Game::EXP62f, fExp / fNeedExp * 100.0);

        g_pRenderText->RenderText(466, 11, Buffer, 0, 0, 1, 0);
    }

    g_pRenderText->RenderText(154, 11, I18N::Game::Lookup(this->ClassNameTextIndex), 0, 0, 1, nullptr);

    g_pRenderText->SetTextColor(255, 155, 0, 0xFFu);

    mu_swprintf(Buffer, I18N::Game::Lookup(this->CategoryTextIndex), this->CategoryPoint[0]);

    g_pRenderText->RenderText(92, 40, Buffer, 0, 0, RT3_SORT_CENTER, 0);

    mu_swprintf(Buffer, I18N::Game::Lookup(this->CategoryTextIndex + 1), this->CategoryPoint[1]);

    g_pRenderText->RenderText(302, 40, Buffer, 0, 0, RT3_SORT_CENTER, 0);

    mu_swprintf(Buffer, I18N::Game::Lookup(this->CategoryTextIndex + 2), this->CategoryPoint[2]);

    g_pRenderText->RenderText(513, 40, Buffer, 0, 0, RT3_SORT_CENTER, 0);
}

void SEASON3B::CNewUIMasterLevel::RenderIcon()
{
    constexpr int SKILL_ICON_WIDTH = 20;
    constexpr int SKILL_ICON_HEIGHT = 28;

    for (auto it = this->map_masterData.begin(); it != this->map_masterData.end(); it++)
    {
        const auto group = it->second.Group;
        const auto skill = it->second.Skill;
        const auto skillAttribute = &SkillAttribute[skill];
        const auto skillLevel = CharacterAttribute->MasterSkillInfo[skill].GetSkillLevel();

        const int index = (it->second.Index - 1) % 4;
        const BYTE rank = skillAttribute->SkillRank;

        const int CalcX = (int)(index * 49.0f + this->categoryPos[group].x);
        const int CalcY = (int)(this->categoryPos[group].y + (skillAttribute->SkillRank - 1) * 41.0f);

        DWORD textColor;

        RenderImage(IMAGE_MASTER_INTERFACE + 4, CalcX, CalcY, 50, 38, 0, 0, 50.f / 64.f, 38.f / 64.f);

        if (!this->CheckParentSkill(it->second)
            || !this->CheckRankPoint(group, rank, skillLevel)
            || !this->CheckBeforeSkill(skill, skillLevel)
            || !g_csItemOption.IsNonWeaponSkillOrIsSkillEquipped(skill)
            )
        {
            textColor = RGBA(120, 120, 120, 255);

            g_pRenderText->SetTextColor(textColor);
            RenderImage(IMAGE_MASTER_INTERFACE + 3, CalcX + 8, CalcY + 5, SKILL_ICON_WIDTH, SKILL_ICON_HEIGHT, (20.f / 512.f) * (skillAttribute->Magic_Icon % 25), ((28.f / 512.f) * ((skillAttribute->Magic_Icon / 25))), 20.f / 512, 28.f / 512.f);
        }
        else
        {
            textColor = RGBA(255, 255, 255, 255);

            g_pRenderText->SetTextColor(textColor);

            RenderImage(IMAGE_MASTER_INTERFACE + 2, CalcX + 8, CalcY + 5, SKILL_ICON_WIDTH, SKILL_ICON_HEIGHT, (20.f / 512.f) * (skillAttribute->Magic_Icon % 25), ((28.f / 512.f) * ((skillAttribute->Magic_Icon / 25))), 20.f / 512.f, 28.f / 512.f);
        }

        if (it->second.ArrowDirection == 1)
            RenderImage(IMAGE_MASTER_INTERFACE + 6, CalcX + 8 + (SKILL_ICON_WIDTH)+2, CalcY + (SKILL_ICON_HEIGHT / 2), 28, 7, 0, 0, 28 / 32.f, 7 / 8.f);
        if (it->second.ArrowDirection == 2)
            RenderImage(IMAGE_MASTER_INTERFACE + 7, CalcX + 8 + (SKILL_ICON_WIDTH)+2, CalcY + (SKILL_ICON_HEIGHT / 2), 28, 7, 0, 0, 28 / 32.f, 7 / 8.f);
        if (it->second.ArrowDirection == 3)
            RenderImage(IMAGE_MASTER_INTERFACE + 8, CalcX + 8 + (SKILL_ICON_WIDTH / 2) - 3.5, CalcY + SKILL_ICON_HEIGHT + 7, 7, 12, 0, 0, 7 / 8.f, 12 / 16.f);
        if (it->second.ArrowDirection == 4)
            RenderImage(IMAGE_MASTER_INTERFACE + 9, CalcX + 8 + (SKILL_ICON_WIDTH / 2) - 3.5, CalcY + SKILL_ICON_HEIGHT + 7, 7, 52, 0, 0, 7 / 8.f, 12 / 16.f);
        if (it->second.ArrowDirection == 5)
            RenderImage(IMAGE_MASTER_INTERFACE + 10, CalcX, CalcY, 42, 31, 0, 0, 42 / 64.f, 31 / 32.f);
        if (it->second.ArrowDirection == 6)
            RenderImage(IMAGE_MASTER_INTERFACE + 11, CalcX, CalcY, 42, 31, 0, 0, 42 / 64.f, 31 / 32.f);
        if (it->second.ArrowDirection == 7)
            RenderImage(IMAGE_MASTER_INTERFACE + 12, CalcX + 8 + (SKILL_ICON_WIDTH / 2) - 1.5, CalcY + SKILL_ICON_HEIGHT + 8, 40, 28, 0, 0, 40 / 64.f, 28 / 32.f);
        if (it->second.ArrowDirection == 8)
            RenderImage(IMAGE_MASTER_INTERFACE + 13, CalcX, CalcY, 40, 28, 0, 0, 40 / 64.f, 28 / 32.f);

        g_pRenderText->RenderText(CalcX + 8 + 30, CalcY + 28 - 5, std::to_wstring(skillLevel).c_str());
    }

    this->RenderToolTip();
}

void SEASON3B::CNewUIMasterLevel::RenderToolTip()
{
    for (auto it = this->map_masterData.begin(); it != this->map_masterData.end(); it++)
    {
        const BYTE group = it->second.Group;

        auto Skill = it->second.Skill;

        SKILL_ATTRIBUTE* p = &SkillAttribute[Skill];

        if (p == nullptr)
        {
            break;
        }

        const int index = (it->second.Index - 1) % 4;

        const int CalcX = (int)(index * 49.0f + this->categoryPos[group].x);

        const int CalcY = (int)(this->categoryPos[group].y + (p->SkillRank - 1) * 41.0f);

        if (SEASON3B::IsPress(VK_LBUTTON) == true || SEASON3B::CheckMouseIn(CalcX + 8, CalcY + 5, 20, 28) == false)
        {
            continue;
        }

        auto mtit = this->map_masterSkillToolTip.find(Skill);

        if (mtit == this->map_masterSkillToolTip.end())
        {
            return;
        }

        const wchar_t* info1 = mtit->second.Info1;
        const wchar_t* info2 = mtit->second.Info2;
        const wchar_t* info3 = mtit->second.Info3;
        const wchar_t* info4 = mtit->second.Info4;
        const wchar_t* info5 = mtit->second.Info5;
        const wchar_t* info6 = mtit->second.Info6;
        const wchar_t* info7 = mtit->second.Info7;
        const auto localized = this->map_localizedMasterSkillToolTip.find(Skill);
        if (localized != this->map_localizedMasterSkillToolTip.end())
        {
            info1 = localized->second.Info1;
            info2 = localized->second.Info2;
            info3 = localized->second.Info3;
            info4 = localized->second.Info4;
            info5 = localized->second.Info5;
            info6 = localized->second.Info6;
            info7 = localized->second.Info7;
        }

        auto skillInfo = CharacterAttribute->MasterSkillInfo[Skill];
        const auto skillLevel = skillInfo.GetSkillLevel();
        auto skillValue = skillInfo.GetSkillValue();
        const auto skillNextValue = skillInfo.GetSkillNextValue();

        for (int i = 0; i < 30; i++)
        {
            TextList[i][0] = 0;
        }

        memset(TextBold, 0, sizeof(TextBold));

        for (int i = 0; i < 30; i++)
        {
            TextListColor[i] = i == 0 ? TEXT_COLOR_YELLOW : TEXT_COLOR_WHITE;
        }

        int lineCount = 0;

        mu_swprintf(TextList[lineCount], L"%ls", g_SkillDataHandler.GetSkillName(Skill));

        TextBold[lineCount] = true;

        lineCount++;

        mu_swprintf(TextList[lineCount], info1, p->SkillRank, skillLevel, it->second.MaxLevel);

        lineCount++;

        wchar_t buffer[512] = {};

        if (it->second.DefValue == -1.0f)
        {
            mu_swprintf(buffer, info2);
        }
        else
        {
            mu_swprintf(buffer, info2, skillLevel != 0 ? skillValue : it->second.DefValue);
        }

        lineCount = this->SetDivideString(buffer, 0, lineCount, 0, 0, true);

        if (skillLevel != 0 && skillLevel < it->second.MaxLevel)
        {
            mu_swprintf(buffer, I18N::Game::NextLevel);

            lineCount = this->SetDivideString(buffer, 0, lineCount, 4, 0, true);

            TextBold[lineCount] = 1;

            mu_swprintf(buffer, info2, skillNextValue);

            lineCount = this->SetDivideString(buffer, 0, lineCount, 0, 0, true);
        }

        if (skillLevel < it->second.MaxLevel)
        {
            mu_swprintf(buffer, I18N::Game::Requirements3329);

            lineCount = this->SetDivideString(buffer, 0, lineCount, 1, 0, true);

            TextBold[lineCount] = 1;

            mu_swprintf(buffer, info3, it->second.RequiredPoints);

            if (it->second.RequiredPoints <= Master_Level_Data.nMLevelUpMPoint)
            {
                lineCount = this->SetDivideString(buffer, 0, lineCount, 0, 0, true);
            }
            else
            {
                lineCount = this->SetDivideString(buffer, 0, lineCount, 2, 0, true);
            }
        }

        int iTextColor = this->CheckBeforeSkill(Skill, skillLevel) == true ? 0 : 2;

        mu_swprintf(buffer, info4);

        lineCount = this->SetDivideString(buffer, 0, lineCount, iTextColor, 0, true);

        if (skillLevel < it->second.MaxLevel && p->SkillRank != 1)
        {
            iTextColor = this->CheckRankPoint(group, p->SkillRank, skillLevel) == true ? 0 : 2;

            mu_swprintf(buffer, info5);

            lineCount = this->SetDivideString(buffer, 0, lineCount, iTextColor, 0, true);

            for (int i = 0; i < MAX_MASTER_SKILL_REQUIRES; i++)
            {
                const auto RequireSkill = it->second.RequireSkill[i];

                if (RequireSkill >= AT_SKILL_MASTER_BEGIN && RequireSkill <= AT_SKILL_MASTER_END)
                {
                    auto requiredSkill = CharacterAttribute->MasterSkillInfo[RequireSkill];
                    iTextColor = requiredSkill.GetSkillValue() < 10 ? 2 : 0;
                    mu_swprintf(buffer, i == 0 ? info6 : info7);
                    lineCount = this->SetDivideString(buffer, 0, lineCount, iTextColor, 0, true);
                }
            }
        }

        if (CalcY > 300)
        {

            RenderTipTextList(CalcX + 8, CalcY + 33, lineCount, 0, 3, STRP_BOTTOMCENTER, 1);
        }
        else
        {
            RenderTipTextList(CalcX + 8, CalcY + 33, lineCount, 0, 3, 0, 1);
        }

    }
}

bool SEASON3B::CNewUIMasterLevel::CheckMouse(int posx, int posy)
{
    constexpr POINT position[3] = { {185,65},{385,65},{585,65} };

    for (int i = 0; i < MAX_MASTER_SKILL_CATEGORY; i++)
    {
        if (SEASON3B::CheckMouseIn(position[i].x + this->PosX, this->ButtonY[i] + position[i].y + this->PosY, 15, 30) == true && this->ButtonX[i] == 0)
        {
            this->ButtonY[i] = 1;

            return false;
        }
    }

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckBtn()
{
    constexpr int posX = 220;

    for (int i = 0; i < MAX_MASTER_SKILL_CATEGORY; i++)
    {
        if (this->ButtonX[i] == 1 && SEASON3B::IsRelease(VK_LBUTTON))
        {
            this->ButtonX[i] = 0;
            return false;
        }

        if (this->ButtonX[i] == 1)
        {
            this->ButtonY[i] = MouseY - 65;

            if (this->ButtonY[i] > posX)
            {
                this->ButtonY[i] = posX;
            }
            else if (this->ButtonY[i] <= 0)
            {
                this->ButtonY[i] = 0;
            }
        }
    }

    for (auto it = this->map_masterData.begin(); it != this->map_masterData.end(); it++)
    {
        auto selectedSkill = it->second;
        
        switch (selectedSkill.Group)
        {
        case 0:
        case 1:
        case 2:
            this->CheckAttributeArea(selectedSkill);
            break;
        }
    }

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckAttributeArea(const _MASTER_SKILLTREE_DATA& skillData)
{
    if (skillData.Group < 0 || skillData.Group >= 3)
    {
        return false;
    }

    const auto lpskill = &SkillAttribute[skillData.Skill];

    if (lpskill == nullptr)
    {
        return false;
    }

    const int tindex = (skillData.Index - 1) % 4;

    const int posX = (int)((double)this->categoryPos[skillData.Group].x + tindex * 49.0);

    const int posY = (int)((double)this->categoryPos[skillData.Group].y + (lpskill->SkillRank - 1) * 41.0);

    if (!SEASON3B::IsPress(VK_LBUTTON) || SEASON3B::CheckMouseIn(posX + 8, posY + 5, 20, 28) == false)
    {
        return true;
    }

    const auto skillPoint = CharacterAttribute->MasterSkillInfo[skillData.Skill].GetSkillLevel();

    PlayBuffer(SOUND_CLICK01);

    if (!this->CheckSkillPoint(Master_Level_Data.nMLevelUpMPoint, skillData, skillPoint))
    {
        return true;
    }

    if (!g_csItemOption.IsNonWeaponSkillOrIsSkillEquipped(skillData.Skill))
    {
        SEASON3B::CreateOkMessageBox(I18N::Game::YouNeedToWearTheRequiredEquipmentToLevelUpThisSkill);
        return true;
    }

    if (!this->CheckParentSkill(skillData)
        || !this->CheckRankPoint(skillData.Group, lpskill->SkillRank, skillPoint)
        || !this->CheckBeforeSkill(skillData.Skill, skillPoint))
    {
        SEASON3B::CreateOkMessageBox(I18N::Game::YouMustMeetAllSkillRequirements);

        return true;
    }

    this->ConsumePoint = skillData.RequiredPoints;
    
    this->CurSkillID = skillData.Skill;

    SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CMaster_Level_Interface));

    MouseLButton = false;

    MouseLButtonPop = false;

    MouseLButtonPush = false;

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckSkillPoint(WORD mLevelUpPoint, const _MASTER_SKILLTREE_DATA& skillData, BYTE skillLevel)
{

    if (skillLevel >= skillData.MaxLevel)
    {
        SEASON3B::CreateOkMessageBox(I18N::Game::YouCanTRaiseAnyMoreLevels);
        return false;
    }

    if (mLevelUpPoint >= skillData.RequiredPoints)
    {
        return true;
    }

    wchar_t Buffer[358] = {};

    mu_swprintf(Buffer, I18N::Game::YouCanTRaiseAnyMoreLevels, skillData.RequiredPoints - mLevelUpPoint);

    SEASON3B::CreateOkMessageBox(Buffer);

    return false;
}

bool SEASON3B::CNewUIMasterLevel::CheckParentSkill(const _MASTER_SKILLTREE_DATA& masterSkill)
{
    for (int i = 0; i < MAX_MASTER_SKILL_REQUIRES; i++)
    {
        const auto requiredSkill = masterSkill.RequireSkill[i];
        if (requiredSkill == AT_SKILL_UNDEFINED)
        {
            return true;
        }

        if (requiredSkill < AT_SKILL_MASTER_BEGIN || requiredSkill > AT_SKILL_MASTER_END)
        {
            return true;
        }

        const auto reqSkillLevel = CharacterAttribute->MasterSkillInfo[requiredSkill].GetSkillLevel();
        if (reqSkillLevel < MASTER_SKILL_LEVEL_REQ_FOR_NEXT_RANK)
        {
            return false;
        }
    }

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckRankPoint(BYTE group, BYTE rank, BYTE skillLevel)
{
    if (this->skillPoint[group][rank] < skillLevel)
    {
        this->skillPoint[group][rank] = skillLevel;
    }

    if (rank == 1)
    {
        return true;
    }

    return this->skillPoint[group][rank - 1] >= 10;
}

bool SEASON3B::CNewUIMasterLevel::CheckBeforeSkill(ActionSkillType skill, BYTE skillLevel)
{
    if (skillLevel != 0)
    {
        return true;
    }

    const auto Index = SkillAttribute[skill].SkillBrand;

    if (Index == 0)
    {
        return true;
    }

    const SKILL_ATTRIBUTE* lpSkill = &SkillAttribute[Index];

    if (lpSkill == nullptr)
    {
        return false;
    }

    if (lpSkill->SkillUseType == 4)
    {
        return true;
    }

    for (int i = 0; i < MAX_MAGIC; i++)
    {
        if (CharacterAttribute->Skill[i] == Index)
        {
            return true;
        }
    }

    return false;
}

void SEASON3B::CNewUIMasterLevel::SkillUpgrade(int index, BYTE skillLevel, float value, float nextValue)
{
    const auto it = this->map_masterData.find(index);
    if (it == this->map_masterData.end())
    {
        return;
    }

    const auto realSkill = it->second.Skill;
    const int oldLevel = CharacterAttribute->MasterSkillInfo[realSkill].GetSkillLevel();

    const CSkillTreeInfo skillTreeInfo = { skillLevel, value, nextValue };
    CharacterAttribute->MasterSkillInfo[realSkill] = skillTreeInfo;

    // And update the category points
    const int addedPoints = skillLevel - oldLevel;
    this->CategoryPoint[it->second.Group] += addedPoints;
}

void SEASON3B::CNewUIMasterLevel::ClearSkillTreeData()
{
    if (!map_masterSkillToolTip.empty())
        this->map_masterData.clear();
}

void SEASON3B::CNewUIMasterLevel::ClearSkillTooltipData()
{
    if (!map_masterSkillToolTip.empty())
        this->map_masterSkillToolTip.clear();
}

void SEASON3B::CNewUIMasterLevel::ClearLocalizedSkillTooltipData()
{
    this->map_localizedMasterSkillToolTipByClass.clear();
    this->map_localizedMasterSkillToolTip.clear();
}
