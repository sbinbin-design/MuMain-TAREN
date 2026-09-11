#include "stdafx.h"
#include "ItemDataHandler.h"
#include "ItemDataLoader.h"
#include "Core/Globals/_struct.h"
#include "Core/Globals/_define.h"
#include "Core/Globals/_crypt.h"
#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/ZzzInfomation.h"

#include <array>
#include <vector>

#ifdef _EDITOR
#include "ItemDataSaver.h"
#include "ItemDataExportS6E3.h"
#include "ItemDataExportAsCSV.h"
#endif

// External references
extern ITEM_ATTRIBUTE* ItemAttribute;

namespace
{
using LegacyItemRecord = std::array<BYTE, sizeof(ITEM_ATTRIBUTE_FILE_LEGACY)>;

// Static audit confirmed these are the same Items with version-balance differences only;
// allow official Name-only localization overlay for these slots.
constexpr std::array<int, 4> kLocalizedNameExceptionSlots = { 19, 1037, 2066, 2570 };

bool IsLocalizedNameExceptionSlot(int itemIndex)
{
    for (const int exceptionSlot : kLocalizedNameExceptionSlots)
    {
        if (exceptionSlot == itemIndex)
            return true;
    }
    return false;
}

bool ReadLegacyItemRecords(const wchar_t* fileName, std::vector<LegacyItemRecord>& records)
{
    FILE* fp = ::_wfopen(fileName, L"rb");
    if (fp == nullptr)
        return false;

    const long expectedSize = static_cast<long>(sizeof(LegacyItemRecord) * MAX_ITEM + sizeof(DWORD));
    ::fseek(fp, 0, SEEK_END);
    const long fileSize = ::ftell(fp);
    ::fseek(fp, 0, SEEK_SET);
    if (fileSize != expectedSize)
    {
        ::fclose(fp);
        return false;
    }

    std::vector<BYTE> buffer(sizeof(LegacyItemRecord) * MAX_ITEM);
    DWORD fileChecksum = 0;
    const bool readBuffer = ::fread(buffer.data(), buffer.size(), 1, fp) == 1;
    const bool readChecksum = ::fread(&fileChecksum, sizeof(fileChecksum), 1, fp) == 1;
    ::fclose(fp);
    if (!readBuffer || !readChecksum ||
        GenerateCheckSum2(buffer.data(), static_cast<DWORD>(buffer.size()), 0xE2F1) != fileChecksum)
    {
        return false;
    }

    records.resize(MAX_ITEM);
    for (int i = 0; i < MAX_ITEM; ++i)
    {
        memcpy(records[i].data(), buffer.data() + i * sizeof(LegacyItemRecord), sizeof(LegacyItemRecord));
        BuxConvert(records[i].data(), static_cast<int>(records[i].size()));
    }
    return true;
}

bool HasItemName(const LegacyItemRecord& record)
{
    return record[0] != '\0';
}

bool SameItemBusinessFields(const LegacyItemRecord& leftRecord, const LegacyItemRecord& rightRecord)
{
    ITEM_ATTRIBUTE_FILE_LEGACY left{};
    ITEM_ATTRIBUTE_FILE_LEGACY right{};
    memcpy(&left, leftRecord.data(), sizeof(left));
    memcpy(&right, rightRecord.data(), sizeof(right));

#define COMPARE_ITEM_FIELD(name, type, arraySize, width, i18nName) \
    if (left.name != right.name) return false;
    ITEM_FIELDS_SIMPLE(COMPARE_ITEM_FIELD)
#undef COMPARE_ITEM_FIELD

    return memcmp(left.RequireClass, right.RequireClass, sizeof(left.RequireClass)) == 0 &&
           memcmp(left.Resistance, right.Resistance, sizeof(left.Resistance)) == 0;
}

int ItemNameLength(const LegacyItemRecord& record)
{
    int length = 0;
    while (length < static_cast<int>(sizeof(((ITEM_ATTRIBUTE_FILE_LEGACY*)nullptr)->Name)) && record[length] != '\0')
        ++length;
    return length;
}
} // namespace

CItemDataHandler::CItemDataHandler()
{
    ClearLocalizedItemNames();
}

CItemDataHandler& CItemDataHandler::GetInstance()
{
    static CItemDataHandler instance;
    return instance;
}

ITEM_ATTRIBUTE* CItemDataHandler::GetItemAttributes()
{
    return ItemAttribute;
}

ITEM_ATTRIBUTE* CItemDataHandler::GetItemAttribute(int index)
{
    if (index >= 0 && index < MAX_ITEM)
        return &ItemAttribute[index];
    return nullptr;
}

const wchar_t* CItemDataHandler::GetItemName(int index) const
{
    if (index < 0 || index >= MAX_ITEM)
        return L"";
    if (GameConfig::GetInstance().GetUILocale() == L"zh-CN" && !m_LocalizedItemNames[index].empty())
        return m_LocalizedItemNames[index].c_str();
    return ItemAttribute[index].Name;
}

int CItemDataHandler::GetItemCount() const
{
    return MAX_ITEM;
}

bool CItemDataHandler::Load(wchar_t* fileName)
{
    return ItemDataLoader::Load(fileName);
}

void CItemDataHandler::ClearLocalizedItemNames()
{
    m_LocalizedItemNames.fill(std::wstring());
}

bool CItemDataHandler::LoadOfficialLocalizedItemNames(const wchar_t* currentItemFileName,
                                                      const wchar_t* officialItemFileName)
{
    ClearLocalizedItemNames();

    std::vector<LegacyItemRecord> currentRecords;
    std::vector<LegacyItemRecord> officialRecords;
    if (!ReadLegacyItemRecords(currentItemFileName, currentRecords) ||
        !ReadLegacyItemRecords(officialItemFileName, officialRecords))
    {
        return false;
    }

    std::array<std::wstring, MAX_ITEM> loadedNames;
    for (int i = 0; i < MAX_ITEM; ++i)
    {
        if (!HasItemName(currentRecords[i]) || !HasItemName(officialRecords[i]))
            continue;

        if (!SameItemBusinessFields(currentRecords[i], officialRecords[i]) &&
            !IsLocalizedNameExceptionSlot(i))
        {
            continue;
        }

        const int nameLength = ItemNameLength(officialRecords[i]);
        if (nameLength == 0)
            continue;

        if (!CMultiLanguage::ConvertFromCodePageToString(loadedNames[i],
                                                         reinterpret_cast<const char*>(officialRecords[i].data()),
                                                         54936u, nameLength))
        {
            continue;
        }
    }

    m_LocalizedItemNames.swap(loadedNames);
    return true;
}

#ifdef _EDITOR
bool CItemDataHandler::Save(wchar_t* fileName, std::string* outChangeLog)
{
    return ItemDataSaver::Save(fileName, outChangeLog);
}

bool CItemDataHandler::ExportAsS6E3(wchar_t* fileName)
{
    return ItemDataExportS6E3::SaveLegacy(fileName);
}

bool CItemDataHandler::ExportToCsv(wchar_t* fileName)
{
    return ItemDataExportAsCSV::ExportToCsv(fileName);
}
#endif
