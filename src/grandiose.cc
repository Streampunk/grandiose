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
#include "node_api.h"

Napi::Value version(const Napi::CallbackInfo &info)
{
  const char *ndiVersion = NDIlib_version();
  return Napi::String::New(info.Env(), ndiVersion);
}

Napi::Value isSupportedCPU(const Napi::CallbackInfo &info)
{
  return Napi::Boolean::New(info.Env(), NDIlib_is_supported_CPU());
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  napi_status status;
  napi_property_descriptor desc[] = {
      DECLARE_NAPI_METHOD("find", find),
      DECLARE_NAPI_METHOD("send", send),
      DECLARE_NAPI_METHOD("receive", receive)};
  status = napi_define_properties(env, exports, 3, desc);

  exports.Set("version", Napi::Function::New(env, version));
  exports.Set("isSupportedCPU", Napi::Function::New(env, isSupportedCPU));

  return exports;
}

NODE_API_MODULE(grandiose, Init)
