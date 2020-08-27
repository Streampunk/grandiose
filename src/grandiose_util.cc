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

// Native libraries
#include <assert.h>
#include <chrono>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

// NAPI
#include "node_api.h"

// NDIlib
#include <Processing.NDI.Lib.h>

// Grandiose util
#include "grandiose_util.h"

/*
 * Convert integer to string (non-standard function)
 * Got this from https://codereview.stackexchange.com/q/164322
 * Because it's a non-standard function
 */
char *itoa(int number, char string_out[], int width)
{
  int i = width;
  do
  {
    string_out[i] = (number > 0) ? number % 10 + '0' : ' ';
    number /= 10;
  } while ((i--) != 0);
  string_out[width + 1] = '\0';
  return string_out;
}

napi_status checkStatus(napi_env env, napi_status status,
                        const char *file, uint32_t line)
{
  napi_status infoStatus, throwStatus;
  const napi_extended_error_info *errorInfo;

  if (status == napi_ok)
  {
    // printf("Received status OK.\n");
    return status;
  }

  infoStatus = napi_get_last_error_info(env, &errorInfo);
  assert(infoStatus == napi_ok);
  // printf(
  //     "NAPI error in file %s on line %i. Error %i: %s\n",
  //     file, line,
  //     errorInfo->error_code, errorInfo->error_message);

  if (status == napi_pending_exception)
  {
    // printf(
    //     "NAPI pending exception. Engine error code: %i\n",
    //     errorInfo->engine_error_code);
    return status;
  }

  char errorCode[20];
  throwStatus = napi_throw_error(env,
                                 itoa(errorInfo->error_code, errorCode, 10), errorInfo->error_message);
  assert(throwStatus == napi_ok);

  return napi_pending_exception; // Expect to be cast to void
}

long long microTime(std::chrono::high_resolution_clock::time_point start)
{
  auto elapsed = std::chrono::high_resolution_clock::now() - start;
  return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

const char *getNapiTypeName(napi_valuetype t)
{
  switch (t)
  {
  case napi_undefined:
    return "undefined";
  case napi_null:
    return "null";
  case napi_boolean:
    return "boolean";
  case napi_number:
    return "number";
  case napi_string:
    return "string";
  case napi_symbol:
    return "symbol";
  case napi_object:
    return "object";
  case napi_function:
    return "function";
  case napi_external:
    return "external";
  default:
    return "unknown";
  }
}

napi_status checkArgs(napi_env env, napi_callback_info info, char *methodName,
                      napi_value *args, size_t argc, napi_valuetype *types)
{

  napi_status status;

  size_t realArgc = argc;
  status = napi_get_cb_info(env, info, &realArgc, args, NULL, NULL);
  if (status != napi_ok)
    return status;

  if (realArgc != argc)
  {
    char errorMsg[100];
    // sprintf(errorMsg, "For method %s, expected %zi arguments and got %zi.",
    //         methodName, argc, realArgc);
    napi_throw_error(env, NULL, errorMsg);
    return napi_pending_exception;
  }

  napi_valuetype t;
  for (int x = 0; x < argc; x++)
  {
    status = napi_typeof(env, args[x], &t);
    if (status != napi_ok)
      return status;
    if (t != types[x])
    {
      char errorMsg[100];
      // sprintf(
      //     errorMsg, "For method %s argument %i, expected type %s and got %s.",
      //     methodName, x + 1, getNapiTypeName(types[x]), getNapiTypeName(t));
      napi_throw_error(env, NULL, errorMsg);
      return napi_pending_exception;
    }
  }

  return napi_ok;
};

// TidyCarrier - Carrier Cleaning
void tidyCarrier(napi_env env, carrier *c)
{
  napi_status status;

  // If has passthru set then delete passthru reference
  if (c->passthru != NULL)
  {
    status = napi_delete_reference(env, c->passthru);
    FLOATING_STATUS;
  }

  // If has request set then delete complete async work
  if (c->_request != NULL)
  {
    status = napi_delete_async_work(env, c->_request);
    FLOATING_STATUS;
  }

  // Finally delete carrier
  delete c;
}

int32_t rejectStatus(napi_env env, carrier *c, char *file, int32_t line)
{
  if (c->status != GRANDIOSE_SUCCESS)
  {
    napi_value errorValue, errorCode, errorMsg;
    napi_status status;
    char errorChars[20];

    if (c->status < GRANDIOSE_ERROR_START)
    {
      const napi_extended_error_info *errorInfo;
      status = napi_get_last_error_info(env, &errorInfo);
      FLOATING_STATUS;

      c->errorMsg = std::string(errorInfo->error_message);
    }

    // printf("Received Error with length: %i\n", c->errorMsg.length());
    // printf("Received Error: %s\n", c->errorMsg.c_str());

    char *extMsg = (char *)malloc(sizeof(char) * c->errorMsg.length());
    // printf("After malloc..\n");
    // sprintf(
    //     extMsg, "In file %s on line %i, found error: %s",
    //     file, line, c->errorMsg.c_str());
    status = napi_create_string_utf8(env, itoa(c->status, errorChars, 10),
                                     NAPI_AUTO_LENGTH, &errorCode);
    FLOATING_STATUS;
    status = napi_create_string_utf8(env, extMsg, NAPI_AUTO_LENGTH, &errorMsg);
    FLOATING_STATUS;
    // printf("After create string..\n");
    status = napi_create_error(env, errorCode, errorMsg, &errorValue);
    FLOATING_STATUS;
    status = napi_reject_deferred(env, c->_deferred, errorValue);
    FLOATING_STATUS;

    //free(extMsg);
    tidyCarrier(env, c);
  }
  return c->status;
}

bool validColorFormat(NDIlib_recv_color_format_e format)
{
  switch (format)
  {
  case NDIlib_recv_color_format_BGRX_BGRA:
  case NDIlib_recv_color_format_UYVY_BGRA:
  case NDIlib_recv_color_format_RGBX_RGBA:
  case NDIlib_recv_color_format_UYVY_RGBA:
  case NDIlib_recv_color_format_fastest:
#ifdef _WIN32
  case NDIlib_recv_color_format_BGRX_BGRA_flipped:
#endif
    return true;
  default:
    return false;
  }
}

bool validBandwidth(NDIlib_recv_bandwidth_e bandwidth)
{
  switch (bandwidth)
  {
  case NDIlib_recv_bandwidth_metadata_only:
  case NDIlib_recv_bandwidth_audio_only:
  case NDIlib_recv_bandwidth_lowest:
  case NDIlib_recv_bandwidth_highest:
    return true;
  default:
    return false;
  }
}

bool validFrameFormat(NDIlib_frame_format_type_e format)
{
  switch (format)
  {
  case NDIlib_frame_format_type_progressive:
  case NDIlib_frame_format_type_interleaved:
  case NDIlib_frame_format_type_field_0:
  case NDIlib_frame_format_type_field_1:
    return true;
  default:
    return false;
  }
}

bool validAudioFormat(Grandiose_audio_format_e format)
{
  switch (format)
  {
  case Grandiose_audio_format_float_32_separate:
  case Grandiose_audio_format_int_16_interleaved:
  case Grandiose_audio_format_float_32_interleaved:
    return true;
  default:
    return false;
  }
}
