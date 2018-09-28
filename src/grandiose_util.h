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

#ifndef GRANDIOSE_UTIL_H
#define GRANDIOSE_UTIL_H

#include <chrono>
#include <stdio.h>
#include <string>
#include <Processing.NDI.Lib.h>
#include "node_api.h"

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }

// Handling NAPI errors - use "napi_status status;" where used
#define CHECK_STATUS if (checkStatus(env, status, __FILE__, __LINE__ - 1) != napi_ok) return nullptr
#define PASS_STATUS if (status != napi_ok) return status

napi_status checkStatus(napi_env env, napi_status status,
  const char * file, uint32_t line);

// High resolution timing
#define HR_TIME_POINT std::chrono::high_resolution_clock::time_point
#define NOW std::chrono::high_resolution_clock::now()
long long microTime(std::chrono::high_resolution_clock::time_point start);

// Argument processing
napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,
  napi_value* args, size_t argc, napi_valuetype* types);

// Async error handling
#define GRANDIOSE_OUT_OF_RANGE 4097
#define GRANDIOSE_ASYNC_FAILURE 4098
#define GRANDIOSE_BUILD_ERROR 4099
#define GRANDIOSE_ALLOCATION_FAILURE 4100
#define GRANDIOSE_RECEIVE_CREATE_FAIL 4101
#define GRANDIOSE_NOT_FOUND 4040
#define GRANDIOSE_NOT_VIDEO 4140
#define GRANDIOSE_SUCCESS 0

struct carrier {
  virtual ~carrier() {}
  napi_ref passthru = nullptr;
  int32_t status = GRANDIOSE_SUCCESS;
  std::string errorMsg;
  long long totalTime;
  napi_deferred _deferred;
  napi_async_work _request;
};

void tidyCarrier(napi_env env, carrier* c);
int32_t rejectStatus(napi_env env, carrier* c, char* file, int32_t line);

#define REJECT_STATUS if (rejectStatus(env, c, __FILE__, __LINE__) != GRANDIOSE_SUCCESS) return;
#define FLOATING_STATUS if (status != napi_ok) { \
  printf("Unexpected N-API status not OK in file %s at line %d value %i.\n", \
    __FILE__, __LINE__ - 1, status); \
}

#define NAPI_THROW_ERROR(msg) { \
  char errorMsg[100]; \
  sprintf(errorMsg, msg); \
  napi_throw_error(env, nullptr, errorMsg); \
  return nullptr; \
}

bool validColorFormat(NDIlib_recv_color_format_e format);
bool validBandwidth(NDIlib_recv_bandwidth_e bandwidth);
bool validFrameFormat(NDIlib_frame_format_type_e format);

#endif // GRANDIOSE_UTIL_H
