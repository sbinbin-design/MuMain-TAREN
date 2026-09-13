#include "stdafx.h"
#include "Core/Text/NameValidation.h"

#include "Data/Translation/MultiLanguage.h"

namespace
{
    constexpr std::size_t AsciiNameMinimum = 4;
    constexpr std::size_t CjkNameMinimum = 2;

    bool IsAsciiNameCharacter(const wchar_t character)
    {
        return (character >= L'A' && character <= L'Z')
            || (character >= L'a' && character <= L'z')
            || (character >= L'0' && character <= L'9');
    }

    bool IsCommonChineseCharacter(const wchar_t character)
    {
        return character >= 0x4E00 && character <= 0x9FFF;
    }

    Core::Text::NameValidationResult ValidateName(const std::wstring& name, const std::size_t byteCapacity,
                                                  const bool useMuChineseEncoding)
    {
        if (!CMultiLanguage::IsValidUtf16(name.c_str()))
            return Core::Text::NameValidationResult::IllegalCharacter;

        bool containsChinese = false;
        for (const wchar_t character : name)
        {
            if (IsAsciiNameCharacter(character))
                continue;
            if (IsCommonChineseCharacter(character))
            {
                containsChinese = true;
                continue;
            }
            return Core::Text::NameValidationResult::IllegalCharacter;
        }

        if (useMuChineseEncoding)
        {
            const auto encodedByteLength = CMultiLanguage::GetMuChineseLegacyByteLength(name.c_str());
            if (!encodedByteLength)
                return Core::Text::NameValidationResult::IllegalCharacter;
            if (*encodedByteLength > byteCapacity)
                return Core::Text::NameValidationResult::TooLong;
        }
        else if (CMultiLanguage::GetUtf8ByteLength(name.c_str()) > byteCapacity)
            return Core::Text::NameValidationResult::TooLong;

        const std::size_t minimumLength = containsChinese ? CjkNameMinimum : AsciiNameMinimum;
        if (name.length() < minimumLength)
            return Core::Text::NameValidationResult::TooShort;

        return Core::Text::NameValidationResult::Valid;
    }
}

namespace Core::Text
{
    NameValidationResult ValidateCharacterName(const std::wstring& name)
    {
        return ValidateName(name, CharacterNameUtf8Capacity, false);
    }

    NameValidationResult ValidateGuildName(const std::wstring& name)
    {
        return ValidateName(name, GuildNameCp936Capacity, true);
    }

    bool IsChatMessageWithinUtf8Limit(const wchar_t* message, const std::size_t byteCapacity)
    {
        return CMultiLanguage::IsValidUtf16(message) && CMultiLanguage::GetUtf8ByteLength(message) <= byteCapacity;
    }
}
