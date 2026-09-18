// MoveCommandData.cpp: implementation of the CMoveCommandData class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MoveCommandData.h"

#include <algorithm>
#include <cstddef>

using namespace SEASON3B;

namespace
{
    constexpr std::size_t MoveReqMapNameCapacity = 32;
    constexpr std::size_t MoveReqConversionCapacity = MoveReqMapNameCapacity + 1;
    using MoveReqMapNameConverter = int32_t (*)(wchar_t*, const char*, int);

    void ConvertMoveReqMapName(wchar_t (&target)[MoveReqMapNameCapacity],
                               const char (&source)[MoveReqMapNameCapacity],
                               MoveReqMapNameConverter converter)
    {
        wchar_t converted[MoveReqConversionCapacity] = {};
        const int convertedLength = converter(converted, source, static_cast<int>(sizeof source));
        const std::size_t copyLength = convertedLength > 0
                                           ? std::min(static_cast<std::size_t>(convertedLength),
                                                      MoveReqMapNameCapacity - 1)
                                           : 0;
        std::copy_n(converted, copyLength, target);
        target[MoveReqMapNameCapacity - 1] = L'\0';
    }
}

#pragma pack(push, 1)
typedef struct
{
    int index;
    char szMainMapName[32];
    char szSubMapName[32];
    int iReqLevel;
    int m_iReqMaxLevel;
    int iReqZen;
    int iGateNum;
} MOVEREQINFO_FILE;
#pragma pack(pop)

CMoveCommandData::CMoveCommandData() {}

CMoveCommandData::~CMoveCommandData()
{
    Release();
}

CMoveCommandData* CMoveCommandData::GetInstance()
{
    static CMoveCommandData s_Instance;
    return &s_Instance;
}

bool CMoveCommandData::Create(const std::wstring& filename, bool useMuChineseLegacy)
{
    FILE* fp = _wfopen(filename.c_str(), L"rb");
    if (fp == NULL)
        return false;

    int count = 0;
    fread(&count, sizeof(int), 1, fp);

    for (int i = 0; i < count; i++)
    {
        auto* pMoveInfoData = new MOVEINFODATA;
        MOVEREQINFO_FILE moveReqInfo{};
        fread(&moveReqInfo, sizeof moveReqInfo, 1, fp);

        // cppcheck-suppress dangerousTypeCast
        BuxConvert((BYTE*)&moveReqInfo, sizeof moveReqInfo);
        pMoveInfoData->_ReqInfo.index = moveReqInfo.index;
        pMoveInfoData->_ReqInfo.iGateNum = moveReqInfo.iGateNum;
        pMoveInfoData->_ReqInfo.iReqLevel = moveReqInfo.iReqLevel;
        pMoveInfoData->_ReqInfo.iReqZen = moveReqInfo.iReqZen;
        pMoveInfoData->_ReqInfo.m_iReqMaxLevel = moveReqInfo.m_iReqMaxLevel;
        auto convertMapName = useMuChineseLegacy ? CMultiLanguage::ConvertFromMuChineseLegacy
                                                 : CMultiLanguage::ConvertFromUtf8;
        ConvertMoveReqMapName(pMoveInfoData->_ReqInfo.szMainMapName, moveReqInfo.szMainMapName, convertMapName);
        ConvertMoveReqMapName(pMoveInfoData->_ReqInfo.szSubMapName, moveReqInfo.szSubMapName, convertMapName);
        m_listMoveInfoData.push_back(pMoveInfoData);
    }
    fclose(fp);

    return true;
}

void CMoveCommandData::Release()
{
    auto li = m_listMoveInfoData.begin();
    for (; li != m_listMoveInfoData.end(); li++)
        delete (*li);
    m_listMoveInfoData.clear();
}

bool CMoveCommandData::OpenMoveReqScript(const std::wstring& filename, bool useMuChineseLegacy)
{
    return CMoveCommandData::GetInstance()->Create(filename, useMuChineseLegacy);
}

int CMoveCommandData::GetNumMoveMap()
{
    if (m_listMoveInfoData.size() > 0)
        return m_listMoveInfoData.size();

    return -1;
}

const CMoveCommandData::MOVEINFODATA* CMoveCommandData::GetMoveCommandDataByIndex(int iIndex)
{
    auto li = m_listMoveInfoData.begin();
    for (; li != m_listMoveInfoData.end(); li++)
    {
        if ((*li)->_ReqInfo.index == iIndex)
        {
            return (*li);
        }
    }
    return 0;
}

const std::list<CMoveCommandData::MOVEINFODATA*>& CMoveCommandData::GetMoveCommandDatalist()
{
    return m_listMoveInfoData;
}
