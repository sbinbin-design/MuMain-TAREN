#pragma once

#include <string>
#include <array>

#include "Data/GameData/SkillData/SkillStructs.h"

// Skill Data Handler - Singleton Facade
// Provides centralized access to skill data operations
class CSkillDataHandler
{
public:
    static CSkillDataHandler& GetInstance();

    // Data Operations - delegates to specialized classes
    bool Load(wchar_t* fileName);
    void ClearLocalizedSkillNames();
    bool LoadOfficialLocalizedSkillNames(const wchar_t* officialSkillFileName);

#ifdef _EDITOR
    bool Save(wchar_t* fileName, std::string* outChangeLog = nullptr);
    bool ExportAsS6E3(wchar_t* fileName);
    bool ExportToCsv(wchar_t* fileName);
#endif

    // Data Access
    SKILL_ATTRIBUTE* GetSkillAttributes();
    SKILL_ATTRIBUTE* GetSkillAttribute(int index);
    int GetSkillCount() const;
    const wchar_t* GetSkillName(int index) const;
    bool HasLocalizedSkillName(int index) const;

private:
    CSkillDataHandler();
    ~CSkillDataHandler() = default;

    // Prevent copying
    CSkillDataHandler(const CSkillDataHandler&) = delete;
    CSkillDataHandler& operator=(const CSkillDataHandler&) = delete;

    std::array<std::wstring, MAX_SKILLS> m_LocalizedSkillNames;
};

#define g_SkillDataHandler CSkillDataHandler::GetInstance()
