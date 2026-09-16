#pragma once

#include <stdio.h>

// Item Data Loading Operations
class ItemDataLoader
{
public:
    static bool Load(wchar_t* fileName, bool useMuChineseLegacy = false);

private:
    static bool LoadLegacyFormat(FILE* fp, long fileSize, bool useMuChineseLegacy);
    static bool LoadNewFormat(FILE* fp, long fileSize, bool useMuChineseLegacy);

    // Template for loading item data with different format structures
    template <typename TFileFormat>
    static bool LoadFormat(FILE* fp, const wchar_t* formatName, bool useMuChineseLegacy);
};
