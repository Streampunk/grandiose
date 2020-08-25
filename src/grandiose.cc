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

#include <chrono>
#include <cstdio>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "node_api.h"
#include "grandiose_util.h"

#include "grandiose_find.h"
#include "grandiose_receive.h"
#include "grandiose_send.h"
#include "grandiose_send_video.h"

napi_value version(napi_env env, napi_callback_info info)
{
  napi_status status;

  const char *ndiVersion = NDIlib_version();
  napi_value result;
  status = napi_create_string_utf8(env, ndiVersion, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value isSupportedCPU(napi_env env, napi_callback_info info)
{
  napi_status status;

  napi_value result;
  status = napi_get_boolean(env, NDIlib_is_supported_CPU(), &result);
  CHECK_STATUS;

  return result;
}

napi_value Init(napi_env env, napi_value exports)
{
  napi_status status;
  napi_property_descriptor desc[] = {
      // Methods in alphabetically order
      DECLARE_NAPI_METHOD("find", find),
      DECLARE_NAPI_METHOD("isSupportedCPU", isSupportedCPU),
      DECLARE_NAPI_METHOD("receive", receive),
      DECLARE_NAPI_METHOD("send", send),
      DECLARE_NAPI_METHOD("sendVideo", sendVideo),
      DECLARE_NAPI_METHOD("version", version)};

  // Get number of properties in array
  int arrSize = *(&desc + 1) - desc;

  // Export properties (methods)
  status = napi_define_properties(env, exports, arrSize, desc);
  CHECK_STATUS;

  return exports;
}

NAPI_MODULE(nodencl, Init)
