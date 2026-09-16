#pragma once

#include "Core/Globals/_define.h"
#include "Data/GameData/ItemData/ItemStructs.h"

#include <string>

class CItemDataHandler
{
public:
    static CItemDataHandler& GetInstance();

    // Data Operations - delegates to specialized classes
    bool Load(wchar_t* fileName, bool useMuChineseLegacy = false);

#ifdef _EDITOR
    bool Save(wchar_t* fileName, std::string* outChangeLog = nullptr);
    bool ExportAsS6E3(wchar_t* fileName);
    bool ExportToCsv(wchar_t* fileName);
#endif

    // Data Access
    ITEM_ATTRIBUTE* GetItemAttributes();
    ITEM_ATTRIBUTE* GetItemAttribute(int index);
    const wchar_t* GetItemName(int index) const;
    int GetItemCount() const;

private:
    CItemDataHandler();
    ~CItemDataHandler() = default;

    // Prevent copying
    CItemDataHandler(const CItemDataHandler&) = delete;
    CItemDataHandler& operator=(const CItemDataHandler&) = delete;

};

#define g_ItemDataHandler CItemDataHandler::GetInstance()
