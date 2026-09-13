// MultiLanguage.cpp: implementation of the CMultiLanguage class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <cstring>
#include <utility>

CMultiLanguage* CMultiLanguage::ms_Singleton = NULL;

CMultiLanguage::CMultiLanguage(std::wstring strSelectedML)
{
    ms_Singleton = this;

    if (wcsicmp(strSelectedML.c_str(), L"ENG") == 0)
    {
        byLanguage = 0;
    }
    else if (wcsicmp(strSelectedML.c_str(), L"POR") == 0)
    {
        byLanguage = 1;
    }
    else if (wcsicmp(strSelectedML.c_str(), L"SPN") == 0)
    {
        byLanguage = 2;
    }
    else
    {
        byLanguage = 0;
    }
}

BYTE CMultiLanguage::GetLanguage()
{
    return byLanguage;
}

/**
 * Converts a multibyte byte buffer to UTF-16 using the specified Windows code page.
 *
 * Reads up to @p maxSourceLength bytes from @p source (which may or may not
 * be null-terminated) and writes the converted UTF-16 characters to @p target.
 * The required number of UTF-16 characters is determined first using
 * MultiByteToWideChar().
 *
 * If a null terminator exists within the processed byte range, it is copied
 * by the conversion. Otherwise, the function appends one when possible.
 *
 * @param target Destination buffer for the UTF-16 output.
 * @param source Multibyte encoded byte buffer.
 * @param codePage Windows code page used to decode @p source.
 * @param maxSourceLength Maximum number of bytes to read from @p source.
 *
 * @return Number of UTF-16 characters written (excluding the null terminator
 *         when present). Returns 0 on failure.
 *
 * @note The caller must ensure @p target is large enough to hold the converted
 *       UTF-16 string plus a terminating null character.
 */
int32_t CMultiLanguage::ConvertFromCodePage(wchar_t* target, const char* source, unsigned int codePage,
                                            int maxSourceLength)
{
    if (target == nullptr || source == nullptr)
    {
        return 0;
    }

    // Determine how many UTF-16 characters are needed
    const int requiredChars = MultiByteToWideChar(codePage, 0, source, maxSourceLength, nullptr, 0);
    if (requiredChars <= 0)
    {
        target[0] = L'\0';
        return 0;
    }

    // Perform the conversion
    int written = MultiByteToWideChar(codePage, 0, source,
                                      maxSourceLength, // read at most this many bytes
                                      target,
                                      requiredChars // assume destination large enough
    );

    if (written <= 0)
    {
        target[0] = L'\0';
        return 0;
    }

    // If the source contained a null terminator within the range,
    // MultiByteToWideChar copies it as well.
    if (written < maxSourceLength)
    {
        target[written] = L'\0';
    }

    return written;
}

int32_t CMultiLanguage::ConvertFromUtf8(wchar_t* target, const char* source, int maxSourceLength)
{
    return ConvertFromCodePage(target, source, CP_UTF8, maxSourceLength);
}

int32_t CMultiLanguage::ConvertFromMuChineseLegacy(wchar_t* target, const char* source, int maxSourceLength)
{
    if (target == nullptr || source == nullptr)
        return 0;

    const int requiredChars = MultiByteToWideChar(MuChineseCodePage, MB_ERR_INVALID_CHARS, source, maxSourceLength,
                                                   nullptr, 0);
    if (requiredChars <= 0)
    {
        target[0] = L'\0';
        return 0;
    }

    const int written = MultiByteToWideChar(MuChineseCodePage, MB_ERR_INVALID_CHARS, source, maxSourceLength, target,
                                            requiredChars);
    if (written <= 0)
    {
        target[0] = L'\0';
        return 0;
    }

    if (maxSourceLength > 0)
        target[written] = L'\0';
    return written;
}

int32_t CMultiLanguage::ConvertToMuChineseLegacy(char* target, const wchar_t* source, int maxTargetLength)
{
    if (target == nullptr || source == nullptr)
        return 0;

    const int requiredBytesWithNull = WideCharToMultiByte(
        MuChineseCodePage, WC_NO_BEST_FIT_CHARS, source, -1, nullptr, 0, nullptr, nullptr);
    if (requiredBytesWithNull <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    if (maxTargetLength > 0 && requiredBytesWithNull > maxTargetLength)
    {
        target[0] = '\0';
        return 0;
    }

    BOOL usedDefaultChar = FALSE;
    const int written = WideCharToMultiByte(
        MuChineseCodePage, WC_NO_BEST_FIT_CHARS, source, -1, target,
        maxTargetLength > 0 ? maxTargetLength : requiredBytesWithNull, nullptr, &usedDefaultChar);
    if (usedDefaultChar)
    {
        target[0] = '\0';
        return 0;
    }
    if (written <= 0)
    {
        target[0] = '\0';
        return 0;
    }
    return written - 1;
}

std::optional<std::size_t> CMultiLanguage::GetMuChineseLegacyByteLength(const wchar_t* source)
{
    if (source == nullptr)
        return std::nullopt;
    if (source[0] == L'\0')
        return 0;

    BOOL usedDefaultChar = FALSE;
    const int byteLength = WideCharToMultiByte(
        MuChineseCodePage, WC_NO_BEST_FIT_CHARS, source, -1, nullptr, 0, nullptr,
        &usedDefaultChar);
    if (byteLength <= 0 || usedDefaultChar)
        return std::nullopt;

    return static_cast<std::size_t>(byteLength - 1);
}

int32_t CMultiLanguage::ConvertFromCodePageBounded(wchar_t* target, std::size_t targetCapacity,
                                                   const char* source, unsigned int codePage,
                                                   int maxSourceLength)
{
    if (target == nullptr || targetCapacity == 0 || source == nullptr)
    {
        return 0;
    }

    const int capacity = static_cast<int>(targetCapacity - 1);
    if (capacity <= 0)
    {
        target[0] = L'\0';
        return 0;
    }

    const int written = MultiByteToWideChar(codePage, 0, source, maxSourceLength, target, capacity);
    if (written <= 0)
    {
        target[0] = L'\0';
        return 0;
    }

    target[written] = L'\0';
    return written;
}

bool CMultiLanguage::ConvertFromCodePageToString(std::wstring& target, const char* source,
                                                 unsigned int codePage, int sourceLength)
{
    target.clear();
    if (source == nullptr || sourceLength < 0)
        return false;
    if (sourceLength == 0)
        return true;

    const int requiredChars = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, source, sourceLength, nullptr, 0);
    if (requiredChars <= 0)
        return false;

    std::wstring converted(requiredChars, L'\0');
    const int written = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, source, sourceLength, converted.data(),
                                            requiredChars);
    if (written != requiredChars)
        return false;

    target = std::move(converted);
    return true;
}

std::size_t CMultiLanguage::GetUtf8ByteLength(const wchar_t* source)
{
    if (source == nullptr || source[0] == L'\0')
        return 0;

    const int byteLength = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, source, -1, nullptr, 0, nullptr, nullptr);
    return byteLength > 0 ? static_cast<std::size_t>(byteLength - 1) : 0;
}

bool CMultiLanguage::IsValidUtf16(const wchar_t* source)
{
    if (source == nullptr || source[0] == L'\0')
        return source != nullptr;

    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, source, -1, nullptr, 0, nullptr, nullptr) > 0;
}

int CMultiLanguage::GetUtf8SafePrefixLength(const char* source, const int maxLength)
{
    if (source == nullptr || maxLength <= 0)
        return 0;

    int offset = 0;
    while (offset < maxLength && source[offset] != '\0')
    {
        const unsigned char lead = static_cast<unsigned char>(source[offset]);
        int sequenceLength = 1;
        if (lead >= 0xC2 && lead <= 0xDF)
            sequenceLength = 2;
        else if (lead >= 0xE0 && lead <= 0xEF)
            sequenceLength = 3;
        else if (lead >= 0xF0 && lead <= 0xF4)
            sequenceLength = 4;
        else if (lead >= 0x80)
            break;

        if (offset + sequenceLength > maxLength)
            break;
        for (int index = 1; index < sequenceLength; ++index)
        {
            if ((static_cast<unsigned char>(source[offset + index]) & 0xC0) != 0x80)
                return offset;
        }
        const unsigned char second = sequenceLength > 1 ? static_cast<unsigned char>(source[offset + 1]) : 0;
        if ((sequenceLength == 3 && ((lead == 0xE0 && second < 0xA0) || (lead == 0xED && second > 0x9F)))
            || (sequenceLength == 4 && ((lead == 0xF0 && second < 0x90) || (lead == 0xF4 && second > 0x8F))))
            return offset;
        offset += sequenceLength;
    }
    return offset;
}

int32_t CMultiLanguage::ConvertToUtf8(char* target, const wchar_t* source, int maxSourceLength)
{
    if (target == nullptr || source == nullptr)
    {
        return 0;
    }

    // In this codebase, maxSourceLength is effectively used as the destination buffer capacity.
    const int requiredBytesWithNull = WideCharToMultiByte(CP_UTF8, 0, source, -1, nullptr, 0, nullptr, nullptr);
    if (requiredBytesWithNull <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    if (maxSourceLength > 0)
    {
        std::string tmp;
        tmp.resize(requiredBytesWithNull);
        const int writtenWithNull =
            WideCharToMultiByte(CP_UTF8, 0, source, -1, tmp.data(), requiredBytesWithNull, nullptr, nullptr);
        if (writtenWithNull <= 0)
        {
            target[0] = '\0';
            return 0;
        }

        const int available = std::max<int>(0, maxSourceLength - 1);
        const int srcLen = std::max<int>(0, writtenWithNull - 1);
        const int copyLen = std::min<int>(srcLen, available);

        if (copyLen > 0)
        {
            std::memcpy(target, tmp.data(), static_cast<size_t>(copyLen));
        }
        target[copyLen] = '\0';
        return copyLen;
    }

    const int requiredBytes = requiredBytesWithNull;
    if (requiredBytes <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    const int written = WideCharToMultiByte(CP_UTF8, 0, source, -1, target, requiredBytes, nullptr, nullptr);
    if (written <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    // When source length is -1, WinAPI includes the null terminator in 'written'.
    return written > 0 ? (written - 1) : 0;
}

WPARAM CMultiLanguage::ConvertFulltoHalfWidthChar(DWORD wParam)
{
    auto Char = (wchar_t)(wParam);

    if (Char >= 0xFF01 && Char <= 0xFF5A)
        wParam -= 0xFEE0;
    else if (Char == 0x3000)
        wParam = 0x0020;

    return wParam;
}
