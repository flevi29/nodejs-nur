#ifndef _NUR_NODE_UTIL_H_
#define _NUR_NODE_UTIL_H_ 1

#include <napi.h>
#include <locale>
#include <codecvt>
#include "nur-main.h"

std::string ws2s(const std::wstring &wstr);

Napi::String NewNapiStr(Napi::Env env, std::wstring wStr);

std::wstring NewWStr(Napi::String napiStr);

Napi::Error NapiNURError(Napi::Env env, const NURError nurErr);

template<typename T>
Napi::Array GetTagsAsNapiValueTemplate(Napi::Env env, std::unique_ptr<T[]> &tags, int length);

Napi::Array GetTagsAsNapiValue(Napi::Env env, std::unique_ptr<NUR_TAG_DATA_EX[]> &tags, int length);

Napi::Array GetTagsAsNapiValue(Napi::Env env, std::unique_ptr<NUR_TAG_DATA[]> &tags, int length);

#endif /* _NUR_NODE_UTIL_H_ */
