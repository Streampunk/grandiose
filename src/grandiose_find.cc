/* Copyright 2018 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cstddef>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "grandiose_util.h"
#include "grandiose_find.h"
#include "util.h"

std::unique_ptr<Napi::FunctionReference> GrandioseFinder::Initialize(const Napi::Env &env, Napi::Object exports)
{
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "GrandioseFinder", {
                                                                InstanceMethod<&GrandioseFinder::Dispose>("dispose", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),

                                                                InstanceMethod<&GrandioseFinder::GetSources>("getCurrentSources", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),

                                                            });

  // Create a persistent reference to the class constructor
  std::unique_ptr<Napi::FunctionReference> constructor = std::make_unique<Napi::FunctionReference>();
  *constructor = Napi::Persistent(func);
  exports.Set("GrandioseFinder", func);

  return constructor;
}

GrandioseFinder::GrandioseFinder(const Napi::CallbackInfo &info) : Napi::ObjectWrap<GrandioseFinder>(info)
{
  if (info.Length() > 0 && !info[0].IsUndefined() && !info[0].IsNull())
  {
    if (!info[0].IsObject())
    {
      Napi::Error::New(info.Env(), "Expected an options object").ThrowAsJavaScriptException();
      return;
    }
    Napi::Object rawOptions = info[0].As<Napi::Object>();

    std::optional<bool> rawShowLocalSources = parseBoolean(info.Env(), rawOptions.Get("showLocalSources"));
    if (rawShowLocalSources.has_value())
    {
      options.showLocalSources = *rawShowLocalSources;
    }
    else
    {
      Napi::Error::New(info.Env(), "options.showLocalSources must be a boolean").ThrowAsJavaScriptException();
      return;
    }

    std::optional<std::string> rawGroups = parseString(info.Env(), rawOptions.Get("groups"));
    if (rawGroups.has_value())
    {
      options.groups = *rawGroups;
    }
    else
    {
      Napi::Error::New(info.Env(), "options.groups must be an array of strings").ThrowAsJavaScriptException();
      return;
    }

    std::optional<std::string> rawExtraIps = parseString(info.Env(), rawOptions.Get("extraIPs"));
    if (rawExtraIps.has_value())
    {
      options.extraIPs = *rawExtraIps;
    }
    else
    {
      Napi::Error::New(info.Env(), "options.extraIPs must be an array of strings").ThrowAsJavaScriptException();
      return;
    }
  }

  NDIlib_find_create_t find_create;
  find_create.show_local_sources = options.showLocalSources;
  find_create.p_groups = options.groups.length() > 0 ? options.groups.c_str() : nullptr;
  find_create.p_extra_ips = options.extraIPs.length() > 0 ? options.extraIPs.c_str() : nullptr;

  handle = NDIlib_find_create2(&find_create);
  if (!handle)
  {
    Napi::Error::New(info.Env(), "Failed to initialize NDI finder").ThrowAsJavaScriptException();
    return;
  }
}

GrandioseFinder::~GrandioseFinder()
{
  cleanup();
}

void GrandioseFinder::cleanup()
{
  if (handle != nullptr)
  {
    NDIlib_find_destroy(handle);
    handle = nullptr;
  }
}

Napi::Value GrandioseFinder::Dispose(const Napi::CallbackInfo &info)
{
  cleanup();

  return info.Env().Null();
}

Napi::Value GrandioseFinder::GetSources(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (!handle)
  {
    Napi::Error::New(info.Env(), "GrandioseFinder has been disposed").ThrowAsJavaScriptException();
    return env.Null();
  }

  uint32_t count = 0;
  const NDIlib_source_t *sources = NDIlib_find_get_current_sources(handle, &count);

  if (!sources || count == 0)
    return Napi::Array::New(env, 0);

  Napi::Array result = Napi::Array::New(env, count);
  for (size_t i = 0; i < count; i++)
  {
    const NDIlib_source_t &source = sources[i];
    result[i] = convertSourceToNapi(env, source);
  }

  return result;
}
