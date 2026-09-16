#include "stdafx.h"
#include "ItemDataHandler.h"
#include "ItemDataLoader.h"
#include "Core/Globals/_struct.h"
#include "Core/Globals/_define.h"
#include "Engine/Object/ZzzInfomation.h"

#ifdef _EDITOR
#include "ItemDataSaver.h"
#include "ItemDataExportS6E3.h"
#include "ItemDataExportAsCSV.h"
#endif

// External references
extern ITEM_ATTRIBUTE* ItemAttribute;

namespace
{
constexpr int kArchangelSword = 19;
constexpr int kArchangelAbsoluteWand = 1037;
constexpr int kArchangelCrossbow = 2066;
constexpr int kArchangelStaff = 2570;

void ApplySimplifiedChineseItemCompatibility()
{
    ItemAttribute[kArchangelSword].DamageMin = 220;
    ItemAttribute[kArchangelSword].DamageMax = 230;
    ItemAttribute[kArchangelSword].WeaponSpeed = 45;

    ItemAttribute[kArchangelAbsoluteWand].DamageMin = 200;
    ItemAttribute[kArchangelAbsoluteWand].DamageMax = 223;
    ItemAttribute[kArchangelAbsoluteWand].WeaponSpeed = 45;
    ItemAttribute[kArchangelAbsoluteWand].MagicPower = 138;

    ItemAttribute[kArchangelCrossbow].DamageMin = 224;
    ItemAttribute[kArchangelCrossbow].DamageMax = 246;
    ItemAttribute[kArchangelCrossbow].WeaponSpeed = 45;

    ItemAttribute[kArchangelStaff].DamageMin = 153;
    ItemAttribute[kArchangelStaff].DamageMax = 165;
    ItemAttribute[kArchangelStaff].WeaponSpeed = 30;
    ItemAttribute[kArchangelStaff].MagicPower = 156;

    // Preserve the established MuMain Ancient default behavior. This is a
    // compatibility normalization, not a claim that TAREN's AttType values
    // are incorrect.
    constexpr int compatibilityAttTypeItems[] = {
        33, 34, 2578,
        3619, 3620, 3622, 3626, 3645,
        4131, 4132, 4133, 4134, 4138, 4157,
        4643, 4644, 4645, 4646, 4650, 4669,
        5155, 5156, 5157, 5158, 5162,
        5667, 5668, 5669, 5670, 5674, 5693,
    };
    for (const int itemIndex : compatibilityAttTypeItems)
        ItemAttribute[itemIndex].AttType = 0;
}
} // namespace

CItemDataHandler::CItemDataHandler()
{
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
    return ItemAttribute[index].Name;
}

int CItemDataHandler::GetItemCount() const
{
    return MAX_ITEM;
}

bool CItemDataHandler::Load(wchar_t* fileName, bool useMuChineseLegacy)
{
    if (!ItemDataLoader::Load(fileName, useMuChineseLegacy))
        return false;
    if (useMuChineseLegacy)
        ApplySimplifiedChineseItemCompatibility();
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
