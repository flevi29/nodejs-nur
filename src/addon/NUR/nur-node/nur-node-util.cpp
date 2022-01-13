#include "nur-node-util.h"

std::string ws2s(const std::wstring &wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

Napi::String NewNapiStr(Napi::Env env, std::wstring wStr) {
    static_assert(sizeof(std::wstring::value_type) == sizeof(std::u16string::value_type),
                  "std::wstring and std::u16string are expected to have the same character size, make sure UNICODE is declared");

    const std::u16string u16str(wStr.begin(), wStr.end());
    return Napi::String::New(env, u16str);
}

std::wstring NewWStr(Napi::String napiStr) {
    static_assert(sizeof(std::wstring::value_type) == sizeof(std::u16string::value_type),
                  "std::wstring and std::u16string are expected to have the same character size, make sure UNICODE is declared");
    auto n = 1;
    auto isLittleEndian = (*(char *) &n == 1);
    auto u16str = napiStr.Utf16Value();

    std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> convLittle;
    std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff>, wchar_t> convBig;
    return isLittleEndian
           ? convLittle.from_bytes(reinterpret_cast<const char *> (&u16str[0]),
                                   reinterpret_cast<const char *> (&u16str[0] + u16str.size()))
           : convBig.from_bytes(reinterpret_cast<const char *> (&u16str[0]),
                                reinterpret_cast<const char *> (&u16str[0] + u16str.size()));
}

Napi::Error NapiNURError(Napi::Env env, const NURError nurErr) {
    const auto err = Napi::Error::New(env, NewNapiStr(env, nurErr.msg));
    err.Value()["name"] = "ReaderError";
    err.Value()["errCode"] = nurErr.code;
    return err;
}

template<typename T>
Napi::Array GetTagsAsNapiValueTemplate(Napi::Env env, std::unique_ptr<T[]> &tags, int length) {
    static_assert(
            std::is_same<T, NUR_TAG_DATA_EX>::value || std::is_same<T, NUR_TAG_DATA>::value,
            "Type must be NUR_TAG_DATA_EX or NUR_TAG_DATA"
    );

    auto JSTags = Napi::Array::New(env, length);
    unsigned int ui = 0;
    for (int i = 0; i < length; i++) {
        auto epc = EPCToString(tags[i].epc, tags[i].epcLen);

        Napi::Object JSTag = Napi::Array::New(env, 2);
        JSTag[0u] = NewNapiStr(env, epc);
        JSTag[1u] = Napi::Number::New(env, tags[i].rssi);
        JSTags[ui++] = JSTag;
    }

    return JSTags;
}

Napi::Array GetTagsAsNapiValue(Napi::Env env, std::unique_ptr<NUR_TAG_DATA_EX[]> &tags, int length) {
    return GetTagsAsNapiValueTemplate(env, tags, length);
}

Napi::Array GetTagsAsNapiValue(Napi::Env env, std::unique_ptr<NUR_TAG_DATA[]> &tags, int length) {
    return GetTagsAsNapiValueTemplate(env, tags, length);
}
