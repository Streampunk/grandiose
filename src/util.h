#pragma once

#include "napi.h"

// TODO: It would be really nice to use std::optional here, but it is not widely enough supported yet to make that viable

bool parseBoolean(const Napi::Env &env, const Napi::Maybe<Napi::Value> &value, bool &output)
{
    Napi::Value rawValue = value.UnwrapOr(env.Null());
    if (rawValue.IsUndefined() || rawValue.IsNull())
    {
        output = false;
        return true;
    }

    if (!rawValue.IsBoolean())
        return false;

    output = rawValue.As<Napi::Boolean>().Value();
    return true;
}

bool parseString(const Napi::Env &env, const Napi::Maybe<Napi::Value> &value, std::string &output)
{
    Napi::Value rawValue = value.UnwrapOr(env.Null());
    if (rawValue.IsUndefined() || rawValue.IsNull())
    {
        output = "";
        return true;
    }

    if (!rawValue.IsString())
        return false;

    output = rawValue.As<Napi::String>().Utf8Value();
    return true;
}

// std::optional<std::vector<std::string>> parseStringArray(const Napi::Env &env, const Napi::Maybe<Napi::Value> &value)
// {
//     Napi::Value rawValue = value.UnwrapOr(env.Null());
//     if (rawValue.IsUndefined() || rawValue.IsNull())
//         return std::vector<std::string>{};

//     if (!rawValue.IsArray())
//         return {};

//     std::vector<std::string> result{};

//     Napi::Array rawArray = rawValue.As<Napi::Array>();
//     size_t arrayLength = rawArray.Length();
//     result.reserve(arrayLength);

//     for (size_t i = 0; i < arrayLength; i++)
//     {
//         Napi::Maybe<Napi::Value> entry = rawArray.Get(i);
//         Napi::Value rawEntry = entry.UnwrapOr(env.Null());

//         // Invalid
//         if (!rawEntry.IsString())
//             return {};

//         result.push_back(rawArray.As<Napi::String>().Utf8Value());
//     }

//     return result;
// }

Napi::Object convertSourceToNapi(const Napi::Env &env, const NDIlib_source_t &source)
{
    Napi::Object object = Napi::Object::New(env);

    object.Set("name", source.p_ndi_name);
    object.Set("urlAddress", source.p_url_address);
    object.Set("ipAddress", source.p_ip_address);

    return object;
}