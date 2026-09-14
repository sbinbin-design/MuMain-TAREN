//*****************************************************************************
// File: UIMapName.cpp
//*****************************************************************************

#include "stdafx.h"
#include "UIMapName.h"
#include "World/MapInfra/MapManager.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Textures/ZzzTexture.h"
#include "Data/GameConfig/GameConfig.h"

#ifdef ASG_ADD_GENS_SYSTEM
#include "Engine/Object/ZzzInventory.h"
#endif	// ASG_ADD_GENS_SYSTEM




#define	UIMN_SHOW_TIME			5000
#define	UIMN_ALPHA_VARIATION	0.015f

#ifdef ASG_ADD_GENS_SYSTEM
#define UIMN_STRIFE_HEIGHT		28.0f
#endif	// ASG_ADD_GENS_SYSTEM

namespace
{
    constexpr float kImageTop = 220.0f;

    bool HasOfficialMapNameImage(const wchar_t* fileName)
    {
        std::wstring officialFileName = L"Data\\Interface\\" + std::wstring(fileName);
        const std::size_t extension = officialFileName.find_last_of(L'.');
        if (extension == std::wstring::npos)
            return false;

        officialFileName.replace(extension, std::wstring::npos, L".OZT");
        FILE* file = _wfopen(officialFileName.c_str(), L"rb");
        if (file == nullptr)
            return false;

        fclose(file);
        return true;
    }

    std::wstring ResolveMapNameImagePath(const wchar_t* fileName)
    {
        if (!GameConfig::GetInstance().IsSimplifiedChineseLocale())
            return L"Local\\" + g_strSelectedML + L"\\ImgsMapName\\" + fileName;

        if (HasOfficialMapNameImage(fileName))
            return L"Interface\\" + std::wstring(fileName);

        return L"Local\\Eng\\ImgsMapName\\" + std::wstring(fileName);
    }
}

CUIMapName::CUIMapName()
{
    InitImgPathMap();
}

CUIMapName::~CUIMapName()
{
}

void CUIMapName::InitImgPathMap()
{
    auto resolve = [](const wchar_t* fileName) { return ResolveMapNameImagePath(fileName); };

    m_mapImgPath[0] = resolve(L"lorencia.tga");
    m_mapImgPath[1] = resolve(L"dungeun.tga");
    m_mapImgPath[2] = resolve(L"devias.tga");
    m_mapImgPath[3] = resolve(L"noria.tga");
    m_mapImgPath[4] = resolve(L"losttower.tga");
    m_mapImgPath[6] = resolve(L"stadium.tga");
    m_mapImgPath[7] = resolve(L"atlans.tga");
    m_mapImgPath[8] = resolve(L"tarcan.tga");
    m_mapImgPath[9] = resolve(L"devilsquare.tga");
    m_mapImgPath[10] = resolve(L"Icarus.tga");
    m_mapImgPath[11] = resolve(L"bloodcastle.tga");
    m_mapImgPath[12] = resolve(L"bloodcastle.tga");
    m_mapImgPath[13] = resolve(L"bloodcastle.tga");
    m_mapImgPath[14] = resolve(L"bloodcastle.tga");
    m_mapImgPath[15] = resolve(L"bloodcastle.tga");
    m_mapImgPath[16] = resolve(L"bloodcastle.tga");
    m_mapImgPath[17] = resolve(L"bloodcastle.tga");
    m_mapImgPath[18] = resolve(L"chaoscastle.tga");
    m_mapImgPath[19] = resolve(L"chaoscastle.tga");
    m_mapImgPath[20] = resolve(L"chaoscastle.tga");
    m_mapImgPath[21] = resolve(L"chaoscastle.tga");
    m_mapImgPath[22] = resolve(L"chaoscastle.tga");
    m_mapImgPath[23] = resolve(L"chaoscastle.tga");
    m_mapImgPath[24] = resolve(L"Kalima.tga");
    m_mapImgPath[25] = resolve(L"Kalima.tga");
    m_mapImgPath[26] = resolve(L"Kalima.tga");
    m_mapImgPath[27] = resolve(L"Kalima.tga");
    m_mapImgPath[28] = resolve(L"Kalima.tga");
    m_mapImgPath[29] = resolve(L"Kalima.tga");
    m_mapImgPath[30] = resolve(L"loren.tga");
    m_mapImgPath[31] = resolve(L"ordeal.tga");

    m_mapImgPath[33] = resolve(L"aida.tga");
    m_mapImgPath[34] = resolve(L"crywolffortress.tga");

    m_mapImgPath[36] = resolve(L"lostkalima.tga");
    m_mapImgPath[37] = resolve(L"kantru.tga");
    m_mapImgPath[38] = resolve(L"kantru.tga");
    m_mapImgPath[39] = resolve(L"kantru.tga");

    m_mapImgPath[41] = resolve(L"BalgasBarrack.tga");
    m_mapImgPath[42] = resolve(L"BalgasRefuge.tga");

    m_mapImgPath[45] = resolve(L"IllusionTemple.tga");
    m_mapImgPath[46] = resolve(L"IllusionTemple.tga");
    m_mapImgPath[47] = resolve(L"IllusionTemple.tga");
    m_mapImgPath[48] = resolve(L"IllusionTemple.tga");
    m_mapImgPath[49] = resolve(L"IllusionTemple.tga");
    m_mapImgPath[50] = resolve(L"IllusionTemple.tga");

    m_mapImgPath[51] = resolve(L"Elbeland.tga");
    m_mapImgPath[52] = resolve(L"bloodcastle.tga");
    m_mapImgPath[53] = resolve(L"chaoscastle.tga");

    m_mapImgPath[56] = resolve(L"SwampOfCalmness.tga");
    m_mapImgPath[57] = resolve(L"mapname_raklion.tga");
    m_mapImgPath[58] = resolve(L"mapname_raklionboss.tga");

    m_mapImgPath[62] = resolve(L"santatown.tga");
    m_mapImgPath[63] = resolve(L"pkfield.tga");
    m_mapImgPath[64] = resolve(L"duelarena.tga");
    m_mapImgPath[65] = resolve(L"doppelganger.tga");
    m_mapImgPath[66] = resolve(L"doppelganger.tga");
    m_mapImgPath[67] = resolve(L"doppelganger.tga");
    m_mapImgPath[68] = resolve(L"doppelganger.tga");
    m_mapImgPath[69] = resolve(L"EmpireGuardian.tga");
    m_mapImgPath[70] = resolve(L"EmpireGuardian.tga");
    m_mapImgPath[71] = resolve(L"EmpireGuardian.tga");
    m_mapImgPath[72] = resolve(L"EmpireGuardian.tga");
    m_mapImgPath[79] = resolve(L"MapName_MarketRolen.tga");

#ifdef ASG_ADD_MAP_KARUTAN
    m_mapImgPath[80] = resolve(L"MapName_Karutan.tga");
    m_mapImgPath[81] = resolve(L"MapName_Karutan.tga");
#endif	// ASG_ADD_MAP_KARUTAN
}

void CUIMapName::Init()
{
    m_eState = HIDE;
    m_nOldWorld = -1;
    m_dwOldTime = ::timeGetTime();
    m_dwDeltaTickSum = 0;
    m_fAlpha = 1.0f;
#ifdef ASG_ADD_GENS_SYSTEM
    m_bStrife = false;
#endif	// ASG_ADD_GENS_SYSTEM
}

void CUIMapName::ShowMapName()
{
    m_eState = FADEIN;
    m_fAlpha = 0.2f;
    m_dwDeltaTickSum = 0;

    if (gMapManager.WorldActive == WD_40AREA_FOR_GM)
    {
        m_eState = HIDE;
        return;
    }

    if (m_nOldWorld != gMapManager.WorldActive)
    {
        wchar_t szImgPath[128];
        ::wcscpy(szImgPath, m_mapImgPath[gMapManager.WorldActive].data());

        DeleteBitmap(BITMAP_INTERFACE_EX + 45);
        LoadBitmap(szImgPath, BITMAP_INTERFACE_EX + 45);

        m_nOldWorld = gMapManager.WorldActive;

#ifdef ASG_ADD_GENS_SYSTEM
        m_bStrife = ::IsStrifeMap(gMapManager.WorldActive);
#endif	// ASG_ADD_GENS_SYSTEM
    }
}

void CUIMapName::Update()
{
    DWORD dwNowTime = ::timeGetTime();
    DWORD dwDeltaTick = dwNowTime - m_dwOldTime;

    switch (m_eState)
    {
    case FADEIN:
        m_fAlpha += UIMN_ALPHA_VARIATION;
        if (1.0f <= m_fAlpha)
        {
            m_eState = SHOW;
            m_fAlpha = 1.0f;
        }
        break;

    case SHOW:
        m_dwDeltaTickSum += dwDeltaTick;
        if (m_dwDeltaTickSum > UIMN_SHOW_TIME)
        {
            m_eState = FADEOUT;
            m_dwDeltaTickSum = 0;
        }
        break;

    case FADEOUT:
        m_fAlpha -= UIMN_ALPHA_VARIATION;
        if (0.0f >= m_fAlpha)
        {
            m_eState = HIDE;
            m_fAlpha = 0.0f;
        }
        break;
    }

    m_dwOldTime = dwNowTime;
}

void CUIMapName::Render()
{
    Update();

    if (HIDE == m_eState)
        return;

    const float imageX = UI::MapName::PhysicalLeft(WindowWidth);
    const float imageY = kImageTop * g_fScreenRate_y;

    ::EnableAlphaTest();

#ifdef ASG_ADD_GENS_SYSTEM
    if (m_bStrife)
        ::RenderBitmap(BITMAP_INTERFACE_EX + 47, imageX, imageY - UIMN_STRIFE_HEIGHT,
            UI::MapName::ImageWidth, UIMN_STRIFE_HEIGHT, 0.0f, 0.0f,
            UI::MapName::ImageWidth / 256.0f,
            UIMN_STRIFE_HEIGHT / 32.0f, false, false, m_fAlpha);
#endif	// ASG_ADD_GENS_SYSTEM
    ::RenderBitmap(BITMAP_INTERFACE_EX + 45, imageX, imageY,
        UI::MapName::ImageWidth, UI::MapName::ImageHeight, 0.0f, 0.0f,
        UI::MapName::ImageWidth / 256.0f,
        UI::MapName::ImageHeight / 128.0f, false, false, m_fAlpha);

    ::DisableAlphaBlend();
}
