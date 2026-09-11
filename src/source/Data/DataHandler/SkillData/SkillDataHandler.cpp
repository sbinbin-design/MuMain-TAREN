#include "stdafx.h"
#include "SkillDataHandler.h"

#include "SkillDataLoader.h"
#include "SkillDataSaver.h"
#include "SkillDataExportS6E3.h"
#include "Data/GameData/SkillData/SkillStructs.h"
#include "Data/GameConfig/GameConfig.h"
#include "Core/Globals/_crypt.h"
#include "Core/Globals/_struct.h"
#include "Core/Globals/_define.h"
#include "Engine/Object/ZzzInfomation.h"

#ifdef _EDITOR
#include "SkillDataExportAsCSV.h"
#endif

#include <array>
#include <vector>

// External references
extern SKILL_ATTRIBUTE* SkillAttribute;

namespace
{
using LegacySkillRecord = std::array<BYTE, sizeof(SKILL_ATTRIBUTE_FILE_LEGACY)>;

constexpr int kFirstMasterSkillSlot = 300;
constexpr int kLastLocalizedMasterSkillSlot = 608;
// These slots were statically audited as the same Skills with version-balance
// differences only; their business data remains the MuMain source of truth.
constexpr std::array<int, 9> kLocalizedNameExceptionSlots = {17, 30, 31, 32, 33, 34, 35, 36, 239};

bool IsLocalizedNameExceptionSlot(int skillIndex)
{
    for (const int exceptionSlot : kLocalizedNameExceptionSlots)
    {
        if (exceptionSlot == skillIndex)
            return true;
    }
    return false;
}

bool ReadLegacySkillRecords(const wchar_t* fileName, std::vector<LegacySkillRecord>& records)
{
    FILE* fp = ::_wfopen(fileName, L"rb");
    if (fp == nullptr)
        return false;

    const long payloadSize = static_cast<long>(sizeof(LegacySkillRecord) * MAX_SKILLS);
    const long expectedFileSize = payloadSize + sizeof(DWORD);
    ::fseek(fp, 0, SEEK_END);
    const long fileSize = ::ftell(fp);
    ::fseek(fp, 0, SEEK_SET);
    if (fileSize != expectedFileSize)
    {
        ::fclose(fp);
        return false;
    }

    std::vector<BYTE> buffer(payloadSize);
    DWORD fileChecksum = 0;
    const bool readPayload = ::fread(buffer.data(), buffer.size(), 1, fp) == 1;
    const bool readChecksum = ::fread(&fileChecksum, sizeof(fileChecksum), 1, fp) == 1;
    ::fclose(fp);
    if (!readPayload || !readChecksum ||
        GenerateCheckSum2(buffer.data(), static_cast<DWORD>(buffer.size()), 0x5A18) != fileChecksum)
    {
        return false;
    }

    records.resize(MAX_SKILLS);
    for (int i = 0; i < MAX_SKILLS; ++i)
    {
        memcpy(records[i].data(), buffer.data() + i * sizeof(LegacySkillRecord), sizeof(LegacySkillRecord));
        BuxConvert(records[i].data(), static_cast<int>(records[i].size()));
    }
    return true;
}

int SkillNameLength(const LegacySkillRecord& record)
{
    int length = 0;
    while (length < static_cast<int>(sizeof(((SKILL_ATTRIBUTE_FILE_LEGACY*)nullptr)->Name)) && record[length] != '\0')
        ++length;
    return length;
}

bool SameSkillBusinessFields(const LegacySkillRecord& officialRecord, const SKILL_ATTRIBUTE& currentSkill)
{
    SKILL_ATTRIBUTE_FILE_LEGACY official{};
    memcpy(&official, officialRecord.data(), sizeof(official));

#define COMPARE_SKILL_FIELD(name, type, arraySize, width, i18nName) \
    if (official.name != currentSkill.name) return false;
    SKILL_FIELDS_SIMPLE(COMPARE_SKILL_FIELD)
#undef COMPARE_SKILL_FIELD

    if (memcmp(official.RequireDutyClass, currentSkill.RequireDutyClass, sizeof(official.RequireDutyClass)) != 0 ||
        memcmp(official.RequireClass, currentSkill.RequireClass, sizeof(official.RequireClass)) != 0)
    {
        return false;
    }

#define COMPARE_SKILL_FIELD(name, type, arraySize, width, i18nName) \
    if (official.name != currentSkill.name) return false;
    SKILL_FIELDS_AFTER_ARRAYS(COMPARE_SKILL_FIELD)
#undef COMPARE_SKILL_FIELD
    return true;
}

bool SameMasterSkillIdentityFields(const LegacySkillRecord& officialRecord, const SKILL_ATTRIBUTE& currentSkill)
{
    SKILL_ATTRIBUTE_FILE_LEGACY official{};
    memcpy(&official, officialRecord.data(), sizeof(official));

    if (official.MasteryType != currentSkill.MasteryType ||
        official.SkillUseType != currentSkill.SkillUseType ||
        official.SkillBrand != currentSkill.SkillBrand ||
        official.KillCount != currentSkill.KillCount ||
        memcmp(official.RequireDutyClass, currentSkill.RequireDutyClass, sizeof(official.RequireDutyClass)) != 0 ||
        memcmp(official.RequireClass, currentSkill.RequireClass, sizeof(official.RequireClass)) != 0 ||
        official.SkillRank != currentSkill.SkillRank ||
        official.Magic_Icon != currentSkill.Magic_Icon ||
        official.TypeSkill != currentSkill.TypeSkill ||
        official.ItemSkill != currentSkill.ItemSkill ||
        official.IsDamage != currentSkill.IsDamage ||
        official.Effect != currentSkill.Effect)
    {
        return false;
    }

    return true;
}
} // namespace

CSkillDataHandler::CSkillDataHandler()
{
    ClearLocalizedSkillNames();
}

CSkillDataHandler& CSkillDataHandler::GetInstance()
{
    static CSkillDataHandler instance;
    return instance;
}

SKILL_ATTRIBUTE* CSkillDataHandler::GetSkillAttributes()
{
    return SkillAttribute;
}

SKILL_ATTRIBUTE* CSkillDataHandler::GetSkillAttribute(int index)
{
    if (index >= 0 && index < MAX_SKILLS)
        return &SkillAttribute[index];
    return nullptr;
}

int CSkillDataHandler::GetSkillCount() const
{
    return MAX_SKILLS;
}

const wchar_t* CSkillDataHandler::GetSkillName(int index) const
{
    if (index < 0 || index >= MAX_SKILLS)
        return L"";
    if (GameConfig::GetInstance().GetUILocale() == L"zh-CN" &&
        index <= kLastLocalizedMasterSkillSlot && !m_LocalizedSkillNames[index].empty())
    {
        return m_LocalizedSkillNames[index].c_str();
    }
    return SkillAttribute[index].Name;
}

bool CSkillDataHandler::HasLocalizedSkillName(int index) const
{
    return index >= 0 && index < MAX_SKILLS && !m_LocalizedSkillNames[index].empty();
}

void CSkillDataHandler::ClearLocalizedSkillNames()
{
    m_LocalizedSkillNames.fill(std::wstring());
}

bool CSkillDataHandler::LoadOfficialLocalizedSkillNames(const wchar_t* officialSkillFileName)
{
    ClearLocalizedSkillNames();

    std::vector<LegacySkillRecord> officialRecords;
    if (!ReadLegacySkillRecords(officialSkillFileName, officialRecords))
        return false;

    std::array<std::wstring, MAX_SKILLS> loadedNames;
    for (int i = 0; i <= kLastLocalizedMasterSkillSlot; ++i)
    {
        const LegacySkillRecord& officialRecord = officialRecords[i];
        if (SkillAttribute[i].Name[0] == L'\0' || SkillNameLength(officialRecord) == 0)
            continue;

        const bool sameIdentity = i < kFirstMasterSkillSlot
            ? SameSkillBusinessFields(officialRecord, SkillAttribute[i]) || IsLocalizedNameExceptionSlot(i)
            : SameMasterSkillIdentityFields(officialRecord, SkillAttribute[i]);
        if (!sameIdentity)
        {
            continue;
        }

        const int nameLength = SkillNameLength(officialRecord);
        if (!CMultiLanguage::ConvertFromCodePageToString(
                loadedNames[i], reinterpret_cast<const char*>(officialRecord.data()), 54936u, nameLength))
        {
            continue;
        }
    }

    m_LocalizedSkillNames.swap(loadedNames);
    return true;
}

bool CSkillDataHandler::Load(wchar_t* fileName)
{
    return SkillDataLoader::Load(fileName);
}

#ifdef _EDITOR
bool CSkillDataHandler::Save(wchar_t* fileName, std::string* outChangeLog)
{
    return SkillDataSaver::Save(fileName, outChangeLog);
}

bool CSkillDataHandler::ExportAsS6E3(wchar_t* fileName)
{
    return SkillDataExportS6E3::SaveLegacy(fileName);
}

bool CSkillDataHandler::ExportToCsv(wchar_t* fileName)
{
    return SkillDataExportAsCSV::ExportToCsv(fileName);
}
#endif
