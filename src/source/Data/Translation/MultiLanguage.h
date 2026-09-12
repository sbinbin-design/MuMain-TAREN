// MultiLanguage.h: interface for the CMultiLanguage class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

class CMultiLanguage
{
private:
    static CMultiLanguage* ms_Singleton;

    BYTE byLanguage;

    CMultiLanguage()
    {
        ms_Singleton = this;
    };

public:
    CMultiLanguage(std::wstring strSelectedML);
    ~CMultiLanguage()
    {
        ms_Singleton = 0;
    };

    BYTE GetLanguage(); // Getters

    WPARAM ConvertFulltoHalfWidthChar(DWORD wParam);

    static int32_t ConvertFromCodePage(wchar_t* target, const char* source, unsigned int codePage,
                                       int maxSourceLength = -1);
    static int32_t ConvertFromCodePageBounded(wchar_t* target, std::size_t targetCapacity, const char* source,
                                              unsigned int codePage, int maxSourceLength = -1);
    static bool ConvertFromCodePageToString(std::wstring& target, const char* source, unsigned int codePage,
                                            int sourceLength);
    static std::size_t GetUtf8ByteLength(const wchar_t* source);
    static bool IsValidUtf16(const wchar_t* source);
    static int GetUtf8SafePrefixLength(const char* source, int maxLength);
    static int32_t ConvertFromUtf8(wchar_t* target, const char* source, int maxSourceLength = -1);
    static int32_t ConvertToUtf8(char* target, const wchar_t* source, int maxSourceLength = -1);

    static CMultiLanguage* GetSingletonPtr()
    {
        return ms_Singleton;
    };
};
