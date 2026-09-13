#pragma once

#include <cstddef>
#include <string>

namespace Core::Text
{
    enum class NameValidationResult
    {
        Valid,
        TooShort,
        IllegalCharacter,
        TooLong,
    };

    constexpr std::size_t CharacterNameUtf8Capacity = 10;
    constexpr std::size_t GuildNameCp936Capacity = 8;

    NameValidationResult ValidateCharacterName(const std::wstring& name);
    NameValidationResult ValidateGuildName(const std::wstring& name);
    bool IsChatMessageWithinUtf8Limit(const wchar_t* message, std::size_t byteCapacity);
}
