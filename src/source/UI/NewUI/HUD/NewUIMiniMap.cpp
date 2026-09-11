// NewUIGuildInfoWindow.cpp: implementation of the CNewUIGuildInfoWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <vector>
#include "I18N/All.h"

#include "UI/NewUI/HUD/NewUIMiniMap.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "Audio/DSPlaySound.h"

#include "Guild/NewUIGuildInfoWindow.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "GameLogic/Items/CSItemOption.h"
#include "Data/GameConfig/GameConfig.h"
#include "World/MapInfra/MapManager.h"

extern BYTE m_OccupationState;

using namespace SEASON3B;

namespace
{
bool IsOfficialCompatibleWorld(const wchar_t* filename)
{
    static constexpr const wchar_t* worlds[] = {
        L"World1",  L"World2",  L"World3",  L"World4",  L"World5",  L"World8",  L"World9",
        L"World11", L"World32", L"World34", L"World35", L"World38", L"World39", L"World42",
        L"World52", L"World57", L"World58", L"World64", L"World82",
    };

    for (const auto* world : worlds)
    {
        if (wcscmp(filename, world) == 0)
            return true;
    }
    return false;
}
} // namespace

SEASON3B::CNewUIMiniMap::CNewUIMiniMap()
{
    m_pNewUIMng = NULL;
}

SEASON3B::CNewUIMiniMap::~CNewUIMiniMap()
{
    Release();
}

bool SEASON3B::CNewUIMiniMap::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MINI_MAP, this);

    LoadBitmap(L"Interface\\mini_map_ui_corner.tga", IMAGE_MINIMAP_INTERFACE + 1, GL_LINEAR);
    LoadBitmap(L"Interface\\mini_map_ui_line.jpg", IMAGE_MINIMAP_INTERFACE + 2, GL_LINEAR);
    LoadBitmap(L"Interface\\mini_map_ui_cha.tga", IMAGE_MINIMAP_INTERFACE + 3, GL_LINEAR);
    LoadBitmap(L"Interface\\mini_map_ui_portal.tga", IMAGE_MINIMAP_INTERFACE + 4, GL_LINEAR);
    LoadBitmap(L"Interface\\mini_map_ui_npc.tga", IMAGE_MINIMAP_INTERFACE + 5, GL_LINEAR);
    LoadBitmap(L"Interface\\mini_map_ui_cancel.tga", IMAGE_MINIMAP_INTERFACE + 6, GL_LINEAR);

    m_BtnExit.ChangeButtonImgState(true, IMAGE_MINIMAP_INTERFACE + 6, false);
    m_BtnExit.ChangeButtonInfo(m_Pos.x + 610, 3, 85, 85);
    m_BtnExit.ChangeToolTipText(&I18N::Game::Close388, true);	// 1002 "�ݱ�"

    SetPos(x, y);

    m_Lenth[0].x = 800;
    m_Lenth[1].x = 1000;
    m_Lenth[2].x = 1200;
    m_Lenth[3].x = 1400;
    m_Lenth[4].x = 1600;
    m_Lenth[5].x = 1800;
    m_Lenth[0].y = 800;
    m_Lenth[1].y = 1000;
    m_Lenth[2].y = 1200;
    m_Lenth[3].y = 1400;
    m_Lenth[4].y = 1600;
    m_Lenth[5].y = 1800;
    m_MiniPos = 0;
    m_bSuccess = false;
    return true;
}

void SEASON3B::CNewUIMiniMap::ClosingProcess()
{
    SocketClient->ToGameServer()->SendCloseNpcRequest();
}

float SEASON3B::CNewUIMiniMap::GetLayerDepth()
{
    return 8.1f;
}

void SEASON3B::CNewUIMiniMap::OpenningProcess()
{
}

void SEASON3B::CNewUIMiniMap::Release()
{
    UnloadImages();

    for (int i = 1; i < 7; i++)
    {
        DeleteBitmap(IMAGE_MINIMAP_INTERFACE + i);
    }

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void SEASON3B::CNewUIMiniMap::SetPos(int x, int y)
{
    m_BtnExit.ChangeButtonInfo(REFERENCE_WIDTH - 27, 3, 30, 25);
}

void SEASON3B::CNewUIMiniMap::SetBtnPos(int Num, float x, float y, float nx, float ny)
{
    m_Btn_Loc[Num][0] = x;
    m_Btn_Loc[Num][1] = y;
    m_Btn_Loc[Num][2] = nx;
    m_Btn_Loc[Num][3] = ny;
}

bool SEASON3B::CNewUIMiniMap::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MINI_MAP))
    {
        if (IsPress(VK_ESCAPE) == true || IsPress(VK_TAB) == true)
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_MINI_MAP);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
    }
    return true;
}

bool SEASON3B::CNewUIMiniMap::Render()
{
    float Rot = 45.f;

    if (m_bSuccess == false)
        return m_bSuccess;

    EnableAlphaTest();
    RenderColor(0, 0, REFERENCE_WIDTH, 430, 0.85f, 1);
    DisableAlphaBlend();
    EnableAlphaTest();

    auto Ty = (float)(((float)Hero->PositionX / 256.f) * m_Lenth[m_MiniPos].y);
    auto Tx = (float)(((float)Hero->PositionY / 256.f) * m_Lenth[m_MiniPos].x);
    float Ty1;
    float Tx1;
    float uvxy = (41.7f / 64.f);
    float uvxy_Line = 8.f / 8.f;
    float Ui_wid = 35.f;
    float Ui_Hig = 6.f;
    float Rot_Loc = 45.f;
    int i = 0;

    RenderBitRotate(IMAGE_MINIMAP_INTERFACE, m_Lenth[m_MiniPos].x - Tx, m_Lenth[m_MiniPos].y - Ty, m_Lenth[m_MiniPos].x, m_Lenth[m_MiniPos].y, Rot);

    int NpcWidth = 15;
    int NpcWidthP = 30;
    for (i = 0; i < MAX_MINI_MAP_DATA; i++)
    {
        if (m_Mini_Map_Data[i].Kind > 0)
        {
            Ty1 = (float)(((float)m_Mini_Map_Data[i].Location[0] / 256.f) * m_Lenth[m_MiniPos].y);
            Tx1 = (float)(((float)m_Mini_Map_Data[i].Location[1] / 256.f) * m_Lenth[m_MiniPos].x);
            Rot_Loc = (float)m_Mini_Map_Data[i].Rotation;

            if (m_Mini_Map_Data[i].Kind == 1) //npc
            {
                if (!(gMapManager.WorldActive == WD_34CRYWOLF_1ST && m_OccupationState > 0) || (m_Mini_Map_Data[i].Location[0] == 228 && m_Mini_Map_Data[i].Location[1] == 48 && gMapManager.WorldActive == WD_34CRYWOLF_1ST))
                    RenderPointRotate(IMAGE_MINIMAP_INTERFACE + 5, Tx1, Ty1, NpcWidth, NpcWidth, m_Lenth[m_MiniPos].x - Tx, m_Lenth[m_MiniPos].y - Ty, m_Lenth[m_MiniPos].x, m_Lenth[m_MiniPos].y, Rot, Rot_Loc, 17.5f / 32.f, 17.5f / 32.f, i);
            }
            else
                if (m_Mini_Map_Data[i].Kind == 2)
                    RenderPointRotate(IMAGE_MINIMAP_INTERFACE + 4, Tx1, Ty1, NpcWidthP, NpcWidthP, m_Lenth[m_MiniPos].x - Tx, m_Lenth[m_MiniPos].y - Ty, m_Lenth[m_MiniPos].x, m_Lenth[m_MiniPos].y, Rot, Rot_Loc, 17.5f / 32.f, 17.5f / 32.f, 100 + i);
        }
        else
            break;
    }

    float Ch_wid = 12;
    RenderImage(IMAGE_MINIMAP_INTERFACE + 3, 325, 230, Ch_wid, Ch_wid, 0.f, 0.f, 17.5f / 32.f, 17.5f / 32.f);

    for (i = 0; i < 25; i++)
    {
        RenderImage(IMAGE_MINIMAP_INTERFACE + 2, i * Ui_wid, 0, Ui_wid, Ui_Hig, 0.f, 1.f, uvxy, -uvxy_Line);
        RenderImage(IMAGE_MINIMAP_INTERFACE + 2, i * Ui_wid, 430 - Ui_Hig, Ui_wid, Ui_Hig, 0.f, 0.f, uvxy, uvxy_Line);
    }
    for (i = 0; i < 20; i++)
    {
        RenderBitmapRotate(IMAGE_MINIMAP_INTERFACE + 2, (Ui_Hig / 2.f), i * (Ui_wid - 3.f), Ui_wid, Ui_Hig, -90.f, 0.f, 0.f, uvxy, uvxy_Line);
        RenderBitmapRotate(IMAGE_MINIMAP_INTERFACE + 2, REFERENCE_WIDTH - (Ui_Hig / 2.f), i * (Ui_wid - 3.f), Ui_wid, Ui_Hig, 90.f, 0.f, 0.f, uvxy, uvxy_Line);
    }

    RenderImage(IMAGE_MINIMAP_INTERFACE + 1, 0, 0, Ui_wid, Ui_wid, 0.f, 0.f, uvxy, uvxy);
    RenderImage(IMAGE_MINIMAP_INTERFACE + 1, REFERENCE_WIDTH - Ui_wid, 0, Ui_wid, Ui_wid, uvxy, 0.f, -uvxy, uvxy);
    RenderImage(IMAGE_MINIMAP_INTERFACE + 1, 0, 430 - Ui_wid, Ui_wid, Ui_wid, 0.f, uvxy, uvxy, -uvxy);
    RenderImage(IMAGE_MINIMAP_INTERFACE + 1, REFERENCE_WIDTH - Ui_wid, 430 - Ui_wid, Ui_wid, Ui_wid, uvxy, uvxy, -uvxy, -uvxy);

    m_BtnExit.Render(true);

    DisableAlphaBlend();

    Check_Btn(MouseX, MouseY);
    return true;
}

bool SEASON3B::CNewUIMiniMap::Update()
{
    return true;
}

void SEASON3B::CNewUIMiniMap::LoadImages(const wchar_t* Filename)
{
    wchar_t Fname[300];
    int i = 0;
    mu_swprintf(Fname, L"Data\\%ls\\mini_map.ozt", Filename);
    FILE* pFile = _wfopen(Fname, L"rb");

    if (pFile == NULL)
    {
        m_bSuccess = false;
        return;
    }
    else
    {
        m_bSuccess = true;
        fclose(pFile);
        mu_swprintf(Fname, L"%ls\\mini_map.tga", Filename);
        LoadBitmap(Fname, IMAGE_MINIMAP_INTERFACE, GL_LINEAR);
    }

    for (i = 0; i < MAX_MINI_MAP_DATA; ++i)
    {
        m_Mini_Map_Data[i].Kind = 0;
    }

    const bool isChineseLocale = GameConfig::GetInstance().GetUILocale() == L"zh-CN";
    const std::wstring overrideFile =
        L"Data\\Local\\zh-CN\\Minimap\\Minimap_" + std::wstring(Filename) + L"_zh-CN.bmd";
    const std::wstring officialFile = L"Data\\" + std::wstring(Filename) + L"\\Minimap.bmd";
    const std::wstring fallbackFile =
        L"Data\\Local\\" + g_strSelectedML + L"\\Minimap\\Minimap_" + Filename + L"_" + g_strSelectedML + L".bmd";

    if (isChineseLocale && IsOfficialCompatibleWorld(Filename) && LoadMiniMapData(officialFile, 54936u))
        return;
    if (isChineseLocale && wcscmp(Filename, L"World81") == 0 && LoadMiniMapData(overrideFile, CP_UTF8))
        return;
    LoadMiniMapData(fallbackFile, CP_UTF8);
}

bool SEASON3B::CNewUIMiniMap::LoadMiniMapData(const std::wstring& filename, unsigned int sourceCodePage)
{
    FILE* fp = _wfopen(filename.c_str(), L"rb");
    if (fp == nullptr)
        return false;

    const int Size = sizeof(MINI_MAP_FILE);
    const long expectedSize = static_cast<long>(Size * MAX_MINI_MAP_DATA + 45 + sizeof(DWORD));
    fseek(fp, 0, SEEK_END);
    const long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fileSize != expectedSize)
    {
        fclose(fp);
        return false;
    }

    std::vector<BYTE> buffer(Size * MAX_MINI_MAP_DATA + 45);
    if (fread(buffer.data(), buffer.size(), 1, fp) != 1)
    {
        fclose(fp);
        return false;
    }

    DWORD checksum = 0;
    if (fread(&checksum, sizeof(checksum), 1, fp) != 1)
    {
        fclose(fp);
        return false;
    }
    fclose(fp);

    if (checksum != GenerateCheckSum2(buffer.data(), static_cast<DWORD>(buffer.size()), 0x2BC1))
        return false;

    std::vector<MINI_MAP> loadedData(MAX_MINI_MAP_DATA);
    BYTE* pSeek = buffer.data();
    for (int i = 0; i < MAX_MINI_MAP_DATA; ++i)
    {
        BuxConvert(pSeek, Size);

        MINI_MAP_FILE current{};
        auto* target = &loadedData[i];
        memcpy(&current, pSeek, Size);
        memcpy(target, pSeek, Size);
        if (CMultiLanguage::ConvertFromCodePageBounded(target->Name, MAX_MINIMAP_NAME, current.Name, sourceCodePage,
                                                       MAX_MINIMAP_NAME) == 0 &&
            current.Name[0] != '\0')
        {
            return false;
        }
        pSeek += Size;
    }

    memcpy(m_Mini_Map_Data, loadedData.data(), sizeof(MINI_MAP) * MAX_MINI_MAP_DATA);
    return true;
}

void SEASON3B::CNewUIMiniMap::UnloadImages()
{
    DeleteBitmap(IMAGE_MINIMAP_INTERFACE);
}

bool SEASON3B::CNewUIMiniMap::UpdateMouseEvent()
{
    bool ret = true;

    if (m_BtnExit.UpdateMouseEvent() == true)
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_MINI_MAP);
        return true;
    }

    if (IsPress(VK_LBUTTON))
    {
        ret = Check_Mouse(MouseX, MouseY);
        if (ret == false)
        {
            PlayBuffer(SOUND_CLICK01);
        }
    }

    if (CheckMouseIn(0, 0, REFERENCE_WIDTH, 430))
    {
        return false;
    }

    return ret;
}

bool SEASON3B::CNewUIMiniMap::Check_Mouse(int mx, int my)
{
    return true;
}

bool SEASON3B::CNewUIMiniMap::Check_Btn(int mx, int my)
{
    int i = 0;
    for (i = 0; i < MAX_MINI_MAP_DATA; i++)
    {
        if (m_Mini_Map_Data[i].Kind > 0)
        {
            if (mx > m_Btn_Loc[i][0] && mx < (m_Btn_Loc[i][0] + m_Btn_Loc[i][2]) && my > m_Btn_Loc[i][1] && my < (m_Btn_Loc[i][1] + m_Btn_Loc[i][3]))
            {
                m_TooltipText = (std::wstring)m_Mini_Map_Data[i].Name;
                g_pRenderText->SetFont(g_hFont);
                const SIZE Fontsize = g_pRenderText->MeasureText(
                    m_TooltipText.c_str(), static_cast<int>(m_TooltipText.size()));

                int x = m_Btn_Loc[i][0] + ((m_Btn_Loc[i][2] / 2) - (Fontsize.cx / 2));
                int y = m_Btn_Loc[i][1] + m_Btn_Loc[i][3] + 2;

                y = m_Btn_Loc[i][1] - (Fontsize.cy + 2);

                DWORD backuptextcolor = g_pRenderText->GetTextColor();
                DWORD backuptextbackcolor = g_pRenderText->GetBgColor();

                g_pRenderText->SetTextColor(RGBA(255, 255, 255, 255));
                g_pRenderText->SetBgColor(RGBA(0, 0, 0, 180));
                g_pRenderText->RenderText(x, y, m_TooltipText.c_str(), Fontsize.cx + 6, 0, RT3_SORT_CENTER);

                g_pRenderText->SetTextColor(backuptextcolor);
                g_pRenderText->SetBgColor(backuptextbackcolor);

                return true;
            }
        }
        else
            break;
    }
    return false;
}
