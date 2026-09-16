#include "stdafx.h"

#include "ItemDataLoader.h"
#include "Data/DataHandler/DataFileIO.h"
#include "Data/GameData/ItemData/ItemStructs.h"
#include "Core/Globals/_struct.h"
#include "Core/Globals/_define.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Events/CSChaosCastle.h"
#include <memory>
#include <sstream>

#ifdef _EDITOR
#include "UI/Console/MuEditorConsoleUI.h"
#include "Core/Utilities/StringUtils.h"
#endif

// External references
extern ITEM_ATTRIBUTE* ItemAttribute;

namespace
{
template <typename TFileFormat>
bool CopyItemAttributeWithEncoding(ITEM_ATTRIBUTE& dest, const TFileFormat& source,
                                   bool useMuChineseLegacy)
{
    int sourceNameLength = 0;
    while (sourceNameLength < static_cast<int>(sizeof(source.Name)) && source.Name[sourceNameLength] != '\0')
        ++sourceNameLength;

    if (sourceNameLength == 0)
    {
        dest.Name[0] = L'\0';
    }
    else if ((useMuChineseLegacy
                  ? CMultiLanguage::ConvertFromMuChineseLegacy(dest.Name, source.Name, sourceNameLength)
                  : CMultiLanguage::ConvertFromUtf8(dest.Name, source.Name, sourceNameLength)) <= 0)
    {
        return false;
    }

    COPY_ITEM_ATTRIBUTE_FIELDS(dest, source);
    return true;
}
} // namespace

bool ItemDataLoader::Load(wchar_t* fileName, bool useMuChineseLegacy)
{
    FILE* fp = _wfopen(fileName, L"rb");
    if (fp == NULL)
    {
        std::wstringstream ss;
        ss << fileName << L" - File not exist.";
        DataFileIO::ReportError(ss.str().c_str());
        return false;
    }

    // Get file size to determine structure version
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    const int LegacySize = sizeof(ITEM_ATTRIBUTE_FILE_LEGACY);
    const long expectedLegacySize = LegacySize * MAX_ITEM + sizeof(DWORD);

    if (useMuChineseLegacy && fileSize != expectedLegacySize)
    {
        DataFileIO::ReportError(L"Official zh-CN item file has an unexpected size.");
        fclose(fp);
        return false;
    }

    bool isLegacyFormat = useMuChineseLegacy || (fileSize == expectedLegacySize);
    bool success = false;

#ifdef _EDITOR
    if (isLegacyFormat)
    {
        g_MuEditorConsoleUI.LogEditor("Detected legacy item format (30-byte names)");
    }
#endif

    if (isLegacyFormat)
    {
        success = LoadLegacyFormat(fp, fileSize, useMuChineseLegacy);
    }
    else
    {
        success = LoadNewFormat(fp, fileSize, useMuChineseLegacy);
    }

    fclose(fp);

#ifdef _EDITOR
    if (success)
    {
        // Count non-empty items (items with names)
        int itemCount = 0;
        for (int i = 0; i < MAX_ITEM; i++)
        {
            if (ItemAttribute[i].Name[0] != L'\0')
            {
                itemCount++;
            }
        }

        wchar_t successMsg[256];
        mu_swprintf(successMsg, L"Loaded %d items from %ls", itemCount, fileName);
        g_MuEditorConsoleUI.LogEditor(StringUtils::WideToNarrow(successMsg));
    }
#endif

    return success;
}

template<typename TFileFormat>
bool ItemDataLoader::LoadFormat(FILE* fp, const wchar_t* formatName, bool useMuChineseLegacy)
{
    const int Size = sizeof(TFileFormat);

    // Configure I/O
    DataFileIO::IOConfig config;
    config.itemSize = Size;
    config.itemCount = MAX_ITEM;
    config.checksumKey = 0xE2F1;
    config.decryptRecord = [](BYTE* data, int size) { BuxConvert(data, size); };

    // Read buffer and checksum
    DWORD dwCheckSum;
    auto buffer = DataFileIO::ReadBuffer(fp, config, &dwCheckSum);
    if (!buffer)
    {
        std::wstringstream ss;
        ss << L"Failed to read item file (" << formatName << L").";
        DataFileIO::ReportError(ss.str().c_str());
        return false;
    }

    // Verify checksum
    if (!DataFileIO::VerifyChecksum(buffer.get(), config, dwCheckSum))
    {
        std::wstringstream ss;
        ss << L"Item file corrupted (" << formatName << L").";
        DataFileIO::ReportError(ss.str().c_str());
        return false;
    }

    // Decrypt buffer
    DataFileIO::DecryptBuffer(buffer.get(), config);

    if (!useMuChineseLegacy)
    {
        BYTE* pSeek = buffer.get();
        for (int i = 0; i < MAX_ITEM; i++)
        {
            TFileFormat source;
            memcpy(&source, pSeek, sizeof(source));
            CopyItemAttributeFromSource(ItemAttribute[i], source);
            pSeek += Size;
        }

        return true;
    }

    auto loadedAttributes = std::make_unique<ITEM_ATTRIBUTE[]>(MAX_ITEM);
    BYTE* pSeek = buffer.get();
    for (int i = 0; i < MAX_ITEM; i++)
    {
        TFileFormat source;
        memcpy(&source, pSeek, sizeof(source));
        if (!CopyItemAttributeWithEncoding(loadedAttributes[i], source, true))
        {
            std::wstringstream ss;
            ss << L"Failed to decode item name at index " << i << L" (" << formatName << L").";
            DataFileIO::ReportError(ss.str().c_str());
            return false;
        }
        pSeek += Size;
    }

    memcpy(ItemAttribute, loadedAttributes.get(), sizeof(ITEM_ATTRIBUTE) * MAX_ITEM);

    return true;
}

bool ItemDataLoader::LoadLegacyFormat(FILE* fp, long fileSize, bool useMuChineseLegacy)
{
    return LoadFormat<ITEM_ATTRIBUTE_FILE_LEGACY>(fp, L"legacy format", useMuChineseLegacy);
}

bool ItemDataLoader::LoadNewFormat(FILE* fp, long fileSize, bool useMuChineseLegacy)
{
    return LoadFormat<ITEM_ATTRIBUTE_FILE>(fp, L"new format", useMuChineseLegacy);
}
