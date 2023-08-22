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

#include <cstdio>
#include <chrono>
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
#include "grandiose_send.h"
#include "grandiose_receive.h"
#include "napi.h"

Napi::Value version(const Napi::CallbackInfo &info)
{
  const char *ndiVersion = NDIlib_version();
  return Napi::String::New(info.Env(), ndiVersion);
}

Napi::Value isSupportedCPU(const Napi::CallbackInfo &info)
{
  return Napi::Boolean::New(info.Env(), NDIlib_is_supported_CPU());
}

struct GrandioseInstanceData
{
  std::unique_ptr<Napi::FunctionReference> finder;
};

Napi::Object Init(Napi::Env env, Napi::Object exports)
{

  // Not required, but "correct" (see the SDK documentation).
  if (!NDIlib_initialize()) // TODO - throw in a way that users can catch
    return exports;

  napi_status status;
  napi_property_descriptor desc[] = {
      DECLARE_NAPI_METHOD("send", send),
      DECLARE_NAPI_METHOD("receive", receive)};
  status = napi_define_properties(env, exports, 2, desc);

  exports.Set("version", Napi::Function::New(env, version));
  exports.Set("isSupportedCPU", Napi::Function::New(env, isSupportedCPU));

  auto finderRef = GrandioseFinder::Initialize(env, exports);

  // Store the constructor as the add-on instance data. This will allow this
  // add-on to support multiple instances of itself running on multiple worker
  // threads, as well as multiple instances of itself running in different
  // contexts on the same thread.
  env.SetInstanceData<GrandioseInstanceData>(new GrandioseInstanceData{
      std::move(finderRef),
  });

  return exports;
}

NODE_API_MODULE(grandiose, Init)
