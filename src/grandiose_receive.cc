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
#include <stddef.h>
#include <inttypes.h>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "grandiose_receive.h"
#include "grandiose_util.h"
#include "grandiose_find.h"

// Upon finalize callback (when ready for garbage collecting)
// then destroy receiver instance
void finalizeReceive(napi_env env, void *data, void *hint)
{
  // printf("Releasing receiver.\n");
  NDIlib_recv_destroy((NDIlib_recv_instance_t)data);
}

// Receive callback
void receiveExecute(napi_env env, void *data)
{
  // Prepare receive carrier based on properties from data
  receiveCarrier *c = (receiveCarrier *)data;

  // Prepare NDIlib receive type
  NDIlib_recv_create_v3_t receiveConfig;
  // Set properties from carrier data
  receiveConfig.source_to_connect_to = *c->source;
  receiveConfig.color_format = c->colorFormat;
  receiveConfig.bandwidth = c->bandwidth;
  receiveConfig.allow_video_fields = c->allowVideoFields;
  receiveConfig.p_ndi_recv_name = c->name;

  // Associate receiver instance on carrier
  c->recv = NDIlib_recv_create_v3(&receiveConfig);
  // Guard if creating NDI receiver failed
  if (!c->recv)
  {
    c->status = GRANDIOSE_RECEIVE_CREATE_FAIL;
    c->errorMsg = "Failed to create NDI receiver.";
    return;
  }

  // Connect the receiver
  NDIlib_recv_connect(c->recv, c->source);
}

// Upon receive callback completed
void receiveComplete(napi_env env, napi_status asyncStatus, void *data)
{
  // Prepare receive carrier with properties based on data
  receiveCarrier *c = (receiveCarrier *)data;

  // printf("Completing some receive creation work.\n");

  // Guard perform async creation was not ok
  if (asyncStatus != napi_ok)
  {
    c->status = asyncStatus;
    c->errorMsg = "Async receiver creation failed to complete.";
  }
  REJECT_STATUS;

  // Create result object value to prepare to return
  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  // Set properties/functions in return object

  // Create external value embedded
  // and attach external value to property on result object
  napi_value embedded;
  c->status = napi_create_external(env, c->recv, finalizeReceive, nullptr, &embedded);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "embedded", embedded);
  REJECT_STATUS;

  // Create function in environment: 'video' to request video frame
  // and attach function as property on result object
  napi_value videoFn;
  c->status = napi_create_function(env,
                                   "video", NAPI_AUTO_LENGTH,
                                   videoReceive, nullptr, &videoFn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "video", videoFn);
  REJECT_STATUS;

  // Create function in environment: 'audio' to request audio frame
  // and attach function as property on result object
  napi_value audioFn;
  c->status = napi_create_function(env,
                                   "audio", NAPI_AUTO_LENGTH,
                                   audioReceive, nullptr, &audioFn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "audio", audioFn);
  REJECT_STATUS;

  // Create function in environment: 'metadata' to request metadata frame
  // and attach function as property on result object
  napi_value metadataFn;
  c->status = napi_create_function(env,
                                   "metadata", NAPI_AUTO_LENGTH,
                                   metadataReceive, nullptr, &metadataFn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "metadata", metadataFn);
  REJECT_STATUS;

  // Create function in environment: 'data' to request genric Data frame
  // and attach function as property on result object
  napi_value dataFn;
  c->status = napi_create_function(env,
                                   "data", NAPI_AUTO_LENGTH,
                                   dataReceive, nullptr, &dataFn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "data", dataFn);
  REJECT_STATUS;

  // Parse source, name and uri properties
  napi_value source, name, uri;
  c->status = napi_create_string_utf8(env, c->source->p_ndi_name, NAPI_AUTO_LENGTH, &name);
  REJECT_STATUS;
  c->status = napi_create_string_utf8(env, c->source->p_url_address, NAPI_AUTO_LENGTH, &uri);
  REJECT_STATUS;
  c->status = napi_create_object(env, &source);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, source, "name", name);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, source, "urlAddress", uri);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "source", source);
  REJECT_STATUS;

  // Set properties based on passed
  // Color format
  napi_value colorFormat;
  c->status = napi_create_int32(env, (int32_t)c->colorFormat, &colorFormat);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "colorFormat", colorFormat);
  REJECT_STATUS;

  // Desired bandwidth type
  napi_value bandwidth;
  c->status = napi_create_int32(env, (int32_t)c->bandwidth, &bandwidth);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "bandwidth", bandwidth);
  REJECT_STATUS;

  // Allow video fields
  napi_value allowVideoFields;
  c->status = napi_get_boolean(env, c->allowVideoFields, &allowVideoFields);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "allowVideoFields", allowVideoFields);
  REJECT_STATUS;

  // In case a name is passed by user then populate this property
  if (c->name != nullptr)
  {
    c->status = napi_create_string_utf8(env, c->name, NAPI_AUTO_LENGTH, &name);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "name", name);
    REJECT_STATUS;
  }

  // Resolve deferred callback
  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  // Cleanup carrier (destroys references)
  tidyCarrier(env, c);
}

napi_value receive(napi_env env, napi_callback_info info)
{
  napi_valuetype type;
  receiveCarrier *c = new receiveCarrier;

  napi_value promise;
  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  // Read arguments passed to promise
  size_t argc = 1;
  napi_value args[1];
  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  if (argc != (size_t)1)
    REJECT_ERROR_RETURN(
        "Receiver must be created with an object containing at least a 'source' property.",
        GRANDIOSE_INVALID_ARGS);

  // Type of first argument
  c->status = napi_typeof(env, args[0], &type);
  REJECT_RETURN;
  bool isArray;
  // Check type of first argument is array
  c->status = napi_is_array(env, args[0], &isArray);
  REJECT_RETURN;

  // Guard if first argument is neither object or array
  if ((type != napi_object) || isArray)
    REJECT_ERROR_RETURN(
        "Single argument must be an object, not an array, containing at least a 'source' property.",
        GRANDIOSE_INVALID_ARGS);

  // Config is first argument
  napi_value config = args[0];

  // Prepare properties
  napi_value source, colorFormat, bandwidth, allowVideoFields, name;
  // source is an object (not an array) with name and urlAddress
  // convert to a native source

  // Extract properties
  c->status = napi_get_named_property(env, config, "source", &source);
  REJECT_RETURN;
  // Type of source
  c->status = napi_typeof(env, source, &type);
  REJECT_RETURN;
  // Is source array
  c->status = napi_is_array(env, source, &isArray);
  REJECT_RETURN;
  // Guard source is not object or array
  if ((type != napi_object) || isArray)
    REJECT_ERROR_RETURN(
        "Source property must be an object and not an array.",
        GRANDIOSE_INVALID_ARGS);

  // Type of name
  napi_value checkType;
  c->status = napi_get_named_property(env, source, "name", &checkType);
  REJECT_RETURN;
  c->status = napi_typeof(env, checkType, &type);
  REJECT_RETURN;

  // Guard type of name is not string
  if (type != napi_string)
    REJECT_ERROR_RETURN(
        "Source property must have a 'name' sub-property that is of type string.",
        GRANDIOSE_INVALID_ARGS);

  c->status = napi_get_named_property(env, source, "urlAddress", &checkType);
  REJECT_RETURN;
  c->status = napi_typeof(env, checkType, &type);
  REJECT_RETURN;
  if (type != napi_string)
    REJECT_ERROR_RETURN(
        "Source property must have a 'urlAddress' sub-property that is of type string.",
        GRANDIOSE_INVALID_ARGS);

  // Populate carrier with new instance
  c->source = new NDIlib_source_t();
  c->status = makeNativeSource(env, source, c->source);
  REJECT_RETURN;

  // Set color format for carrier based on config
  c->status = napi_get_named_property(env, config, "colorFormat", &colorFormat);
  REJECT_RETURN;
  c->status = napi_typeof(env, colorFormat, &type);
  REJECT_RETURN;
  if (type != napi_undefined)
  {
    // Guard color format is not number
    if (type != napi_number)
      REJECT_ERROR_RETURN(
          "Color format property must be a number.",
          GRANDIOSE_INVALID_ARGS);

    // Get  enum (numeric) value of color format
    int32_t enumValue;
    c->status = napi_get_value_int32(env, colorFormat, &enumValue);
    REJECT_RETURN;

    // Set color format from numeric value
    c->colorFormat = (NDIlib_recv_color_format_e)enumValue;
    if (!validColorFormat(c->colorFormat))
      REJECT_ERROR_RETURN(
          "Invalid colour format value.",
          GRANDIOSE_INVALID_ARGS);
  }

  // Bandwidth property
  c->status = napi_get_named_property(env, config, "bandwidth", &bandwidth);
  REJECT_RETURN;
  c->status = napi_typeof(env, bandwidth, &type);
  REJECT_RETURN;

  // If bandwidth property set
  if (type != napi_undefined)
  {
    // Guard bandwidth is not a number
    if (type != napi_number)
      REJECT_ERROR_RETURN(
          "Bandwidth property must be a number.",
          GRANDIOSE_INVALID_ARGS);

    int32_t enumValue;
    c->status = napi_get_value_int32(env, bandwidth, &enumValue);
    REJECT_RETURN;

    c->bandwidth = (NDIlib_recv_bandwidth_e)enumValue;
    // Guard validate bandwidth enum value
    if (!validBandwidth(c->bandwidth))
      REJECT_ERROR_RETURN(
          "Invalid bandwidth value.",
          GRANDIOSE_INVALID_ARGS);
  }

  // Allow video fields (whether is interlaced) property
  c->status = napi_get_named_property(env, config, "allowVideoFields", &allowVideoFields);
  REJECT_RETURN;
  c->status = napi_typeof(env, allowVideoFields, &type);
  REJECT_RETURN;
  // If allow video fields property set
  if (type != napi_undefined)
  {
    // Guard invalid property type
    if (type != napi_boolean)
      REJECT_ERROR_RETURN(
          "Allow video fields property must be a Boolean.",
          GRANDIOSE_INVALID_ARGS);
    c->status = napi_get_value_bool(env, allowVideoFields, &c->allowVideoFields);
    REJECT_RETURN;
  }

  // Name property
  c->status = napi_get_named_property(env, config, "name", &name);
  REJECT_RETURN;
  c->status = napi_typeof(env, name, &type);
  // If property is set
  if (type != napi_undefined)
  {
    // Guard name is not string
    if (type != napi_string)
      REJECT_ERROR_RETURN(
          "Optional name property must be a string when present.",
          GRANDIOSE_INVALID_ARGS);

    // Parse name value
    size_t namel;
    c->status = napi_get_value_string_utf8(env, name, nullptr, 0, &namel);
    REJECT_RETURN;
    c->name = (char *)malloc(namel + 1);
    c->status = napi_get_value_string_utf8(env, name, c->name, namel + 1, &namel);
    REJECT_RETURN;
  }

  napi_value resource_name;
  c->status = napi_create_string_utf8(env, "Receive", NAPI_AUTO_LENGTH, &resource_name);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resource_name, receiveExecute,
                                     receiveComplete, c, &c->_request);
  REJECT_RETURN;

  // Perform async work
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

// Video receive callback
void videoReceiveExecute(napi_env env, void *data)
{
  // Data carrier based on passed data
  dataCarrier *c = (dataCarrier *)data;

  // Switch statement for captured frame - return error if not desired type
  switch (NDIlib_recv_capture_v2(c->recv, &c->videoFrame, nullptr, nullptr, c->wait))
  {
  case NDIlib_frame_type_none:
    // printf("No data received.\n");
    // Setting carrier status and error message
    c->status = GRANDIOSE_NOT_FOUND;
    c->errorMsg = "No video data received in the requested time interval.";
    break;

  // Video data
  case NDIlib_frame_type_video:
    // printf(
    //     "Video data %i received (%dx%d at %d/%d).\n",
    //     &c->videoFrame,
    //     c->videoFrame.xres, c->videoFrame.yres,
    //     c->videoFrame.frame_rate_N, c->videoFrame.frame_rate_D);
    break;

  default:
    // printf("Other kind of data received.\n");
    // Setting carrier status and error message
    c->status = GRANDIOSE_NOT_VIDEO;
    c->errorMsg = "Non-video data received on video capture.";
    break;
  }
}

// Upon promise complete for Video frame receive
void videoReceiveComplete(napi_env env, napi_status asyncStatus, void *data)
{
  // Data carrier based on passed data
  dataCarrier *c = (dataCarrier *)data;

  // Guard async status not ok
  if (asyncStatus != napi_ok)
  {
    c->status = asyncStatus;
    c->errorMsg = "Async video frame receive failed to complete.";
  }
  REJECT_STATUS;

  // Guard video frame received invalid resolution
  if (c->videoFrame.xres == 0 || c->videoFrame.yres == 0)
  {
    c->status = GRANDIOSE_RECEIVE_CREATE_FAIL;
    c->errorMsg = "Video frame received has invalid resolution.";
    printf("Invalid video frame size: %ix%i\n", c->videoFrame.xres, c->videoFrame.yres);
    return;
  }
  REJECT_STATUS;

  // Debug
  // printf("Video frame size: %ix%i\n", c->videoFrame.xres, c->videoFrame.yres);

  // Prepare result object for promise
  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  // Parse precise timestamp from videoFrame timestamp
  int32_t ptps, ptpn;
  ptps = (int32_t)(c->videoFrame.timestamp / 10000000);
  ptpn = (c->videoFrame.timestamp % 10000000) * 100;

  // Param to reuse to parse values to properties for result object
  napi_value param;

  // Parse type property (with string 'video' as value) to result object
  c->status = napi_create_string_utf8(env, "video", NAPI_AUTO_LENGTH, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", param);
  REJECT_STATUS;

  // Parse xres (resolution width) property to result object
  c->status = napi_create_int32(env, c->videoFrame.xres, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "xres", param);
  REJECT_STATUS;

  // Parse yres (resolution height) property to result object
  c->status = napi_create_int32(env, c->videoFrame.yres, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "yres", param);
  REJECT_STATUS;

  // Parse framerate numerator property to result object
  c->status = napi_create_int32(env, c->videoFrame.frame_rate_N, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frameRateN", param);
  REJECT_STATUS;

  // Parse framerate denominator property to result object
  c->status = napi_create_int32(env, c->videoFrame.frame_rate_D, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frameRateD", param);
  REJECT_STATUS;

  // Parse picture aspect ratio property to result object
  c->status = napi_create_double(env, (double)c->videoFrame.picture_aspect_ratio, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "pictureAspectRatio", param);
  REJECT_STATUS;

  // Parse timestamp property as array to result object
  napi_value params, paramn;
  c->status = napi_create_int32(env, ptps, &params);
  REJECT_STATUS;
  c->status = napi_create_int32(env, ptpn, &paramn);
  REJECT_STATUS;
  c->status = napi_create_array(env, &param);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 0, params);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 1, paramn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "timestamp", param);
  REJECT_STATUS;

  // Parse frameFormatType property to result object
  c->status = napi_create_int32(env, c->videoFrame.frame_format_type, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frameFormatType", param);
  REJECT_STATUS;

  // Parse timecode property as array to result object
  c->status = napi_create_int32(env, (int32_t)c->videoFrame.timecode / 10000000, &params);
  REJECT_STATUS;
  c->status = napi_create_int32(env, (c->videoFrame.timecode % 10000000) * 100, &paramn);
  REJECT_STATUS;
  c->status = napi_create_array(env, &param);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 0, params);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 1, paramn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "timecode", param);
  REJECT_STATUS;

  // Parse lineStrideInBytes property to result object
  c->status = napi_create_int32(env, c->videoFrame.line_stride_in_bytes, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "lineStrideBytes", param);
  REJECT_STATUS;

  // If metadata for videoFrame is set, then parse metadata
  if (c->videoFrame.p_metadata != nullptr)
  {
    c->status = napi_create_string_utf8(env, c->videoFrame.p_metadata, NAPI_AUTO_LENGTH, &param);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "metadata", param);
    REJECT_STATUS;

    // printf("Had metadata!\n");
  }

  // printf("Video frame data: %s\n", c->videoFrame.p_data);
  // printf("Video frame size: %ix%i\n", c->videoFrame.xres, c->videoFrame.yres);
  // printf("Video frame line stride: %i\n", c->videoFrame.line_stride_in_bytes);

  // Resolve buffer size based on line_stride_in_bytes (number of bytes used per line)
  // and height of image
  // Somehow these videoFrame values can be zero = throws segmentation fault
  int32_t bufferSize = c->videoFrame.line_stride_in_bytes * c->videoFrame.yres;

  // printf("Buffersize: %i\n", bufferSize);

  // Copy buffer of videoFrame data
  c->status = napi_create_buffer_copy(env, bufferSize,
                                      (void *)c->videoFrame.p_data, nullptr, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "data", param);
  REJECT_STATUS;

  // Free video frame
  NDIlib_recv_free_video_v2(c->recv, &c->videoFrame);

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  // Cleanup
  tidyCarrier(env, c);
}

napi_value videoReceive(napi_env env, napi_callback_info info)
{
  napi_valuetype type;
  dataCarrier *c = new dataCarrier;

  napi_value promise;
  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];
  napi_value thisValue;
  c->status = napi_get_cb_info(env, info, &argc, args, &thisValue, nullptr);
  REJECT_RETURN;

  napi_value recvValue;
  c->status = napi_get_named_property(env, thisValue, "embedded", &recvValue);
  REJECT_RETURN;
  void *recvData;
  c->status = napi_get_value_external(env, recvValue, &recvData);
  c->recv = (NDIlib_recv_instance_t)recvData;
  REJECT_RETURN;

  if (argc >= 1)
  {
    // Read typeof first argument
    c->status = napi_typeof(env, args[0], &type);
    REJECT_RETURN;
    // Guard type of first argument must be number
    if (type == napi_number)
    {
      // Set carrier property wait to value from first argument
      c->status = napi_get_value_uint32(env, args[0], &c->wait);
      REJECT_RETURN;
    }
  }

  napi_value resource_name;
  c->status = napi_create_string_utf8(env, "VideoReceive", NAPI_AUTO_LENGTH, &resource_name);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resource_name,
                                     videoReceiveExecute, videoReceiveComplete,
                                     c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

void audioReceiveExecute(napi_env env, void *data)
{
  dataCarrier *c = (dataCarrier *)data;

  // printf("Audio receiver executing.\n");

  switch (NDIlib_recv_capture_v2(c->recv, nullptr, &c->audioFrame, nullptr, c->wait))
  {
  case NDIlib_frame_type_none:
    // printf("No data received.\n");
    c->status = GRANDIOSE_NOT_FOUND;
    c->errorMsg = "No audio data received in the requested time interval.";
    break;

  // Audio data
  case NDIlib_frame_type_audio:
    switch (c->audioFormat)
    {
    case Grandiose_audio_format_int_16_interleaved:
      c->audioFrame16s.reference_level = c->referenceLevel;
      c->audioFrame16s.p_data = new short[c->audioFrame.no_samples * c->audioFrame.no_channels];
      NDIlib_util_audio_to_interleaved_16s_v2(&c->audioFrame, &c->audioFrame16s);
      break;
    case Grandiose_audio_format_float_32_interleaved:
      c->audioFrame32fIlvd.p_data = new float[c->audioFrame.no_samples * c->audioFrame.no_channels];
      NDIlib_util_audio_to_interleaved_32f_v2(&c->audioFrame, &c->audioFrame32fIlvd);
      break;
    case Grandiose_audio_format_float_32_separate:
    default:
      break;
    }
    break;

  default:
    // printf("Other kind of data received.\n");
    c->status = GRANDIOSE_NOT_AUDIO;
    c->errorMsg = "Non-audio data received on audio capture.";
    break;
  }
}

void audioReceiveComplete(napi_env env, napi_status asyncStatus, void *data)
{
  dataCarrier *c = (dataCarrier *)data;

  // printf("Audio receiver completing - status %i.\n", c->status);

  if (asyncStatus != napi_ok)
  {
    c->status = asyncStatus;
    c->errorMsg = "Async audio frame receive failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  int32_t ptps, ptpn;
  ptps = (int32_t)(c->audioFrame.timestamp / 10000000);
  ptpn = (c->audioFrame.timestamp % 10000000) * 100;

  napi_value param;
  c->status = napi_create_string_utf8(env, "audio", NAPI_AUTO_LENGTH, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->audioFormat, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "audioFormat", param);
  REJECT_STATUS;

  if (c->audioFormat == Grandiose_audio_format_int_16_interleaved)
  {
    c->status = napi_create_int32(env, c->referenceLevel, &param);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "referenceLevel", param);
    REJECT_STATUS;
  }

  c->status = napi_create_int32(env, c->audioFrame.sample_rate, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "sampleRate", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->audioFrame.no_channels, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "channels", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->audioFrame.no_samples, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "samples", param);
  REJECT_STATUS;

  int32_t factor = (c->audioFormat == Grandiose_audio_format_int_16_interleaved) ? 2 : 1;
  c->status = napi_create_int32(env, c->audioFrame.channel_stride_in_bytes / factor, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "channelStrideInBytes", param);
  REJECT_STATUS;

  napi_value params, paramn;
  c->status = napi_create_int32(env, ptps, &params);
  REJECT_STATUS;
  c->status = napi_create_int32(env, ptpn, &paramn);
  REJECT_STATUS;
  c->status = napi_create_array(env, &param);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 0, params);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 1, paramn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "timestamp", param);
  REJECT_STATUS;

  // printf("Timecode is %lld.\n", c->audioFrame.timecode);
  c->status = napi_create_int32(env, (int32_t)(c->audioFrame.timecode / 10000000), &params);
  REJECT_STATUS;
  c->status = napi_create_int32(env, (c->audioFrame.timecode % 10000000) * 100, &paramn);
  REJECT_STATUS;
  c->status = napi_create_array(env, &param);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 0, params);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 1, paramn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "timecode", param);
  REJECT_STATUS;

  if (c->audioFrame.p_metadata != nullptr)
  {
    c->status = napi_create_string_utf8(env, c->audioFrame.p_metadata, NAPI_AUTO_LENGTH, &param);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "metadata", param);
    REJECT_STATUS;
  }

  char *rawFloats;
  switch (c->audioFormat)
  {
  case Grandiose_audio_format_int_16_interleaved:
    rawFloats = (char *)c->audioFrame16s.p_data;
    break;
  case Grandiose_audio_format_float_32_interleaved:
    rawFloats = (char *)c->audioFrame32fIlvd.p_data;
    break;
  default:
  case Grandiose_audio_format_float_32_separate:
    rawFloats = (char *)c->audioFrame.p_data;
    break;
  }
  c->status = napi_create_buffer_copy(env,
                                      (c->audioFrame.channel_stride_in_bytes / factor) * c->audioFrame.no_channels,
                                      rawFloats, nullptr, &param);
  REJECT_STATUS;

  c->status = napi_set_named_property(env, result, "data", param);
  REJECT_STATUS;

  NDIlib_recv_free_audio_v2(c->recv, &c->audioFrame);

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value dataAndAudioReceive(napi_env env, napi_callback_info info,
                               char *resourceName, napi_async_execute_callback execute,
                               napi_async_complete_callback complete)
{
  napi_valuetype type;
  dataCarrier *c = new dataCarrier;

  napi_value promise;
  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 2;
  napi_value args[2];
  napi_value thisValue;
  c->status = napi_get_cb_info(env, info, &argc, args, &thisValue, nullptr);
  REJECT_RETURN;

  napi_value recvValue;
  c->status = napi_get_named_property(env, thisValue, "embedded", &recvValue);
  REJECT_RETURN;
  void *recvData;
  c->status = napi_get_value_external(env, recvValue, &recvData);
  c->recv = (NDIlib_recv_instance_t)recvData;
  REJECT_RETURN;

  if (argc >= 1)
  {
    napi_value configValue, waitValue;
    configValue = args[0];
    c->status = napi_typeof(env, configValue, &type);
    REJECT_RETURN;
    waitValue = (type == napi_number) ? args[0] : args[1];
    if (type == napi_object)
    {
      bool isArray;
      c->status = napi_is_array(env, configValue, &isArray);
      REJECT_RETURN;
      if (isArray)
        REJECT_ERROR_RETURN(
            "First argument to audio receive cannot be an array.",
            GRANDIOSE_INVALID_ARGS);

      napi_value param;
      c->status = napi_get_named_property(env, configValue, "audioFormat", &param);
      REJECT_RETURN;
      c->status = napi_typeof(env, param, &type);
      REJECT_RETURN;
      if (type == napi_number)
      {
        uint32_t audioFormatN;
        c->status = napi_get_value_uint32(env, param, &audioFormatN);
        REJECT_RETURN;
        if (!validAudioFormat((Grandiose_audio_format_e)audioFormatN))
          REJECT_ERROR_RETURN(
              "Invalid audio format specified.", GRANDIOSE_INVALID_ARGS);
        c->audioFormat = (Grandiose_audio_format_e)audioFormatN;
      }
      else if (type != napi_undefined)
        REJECT_ERROR_RETURN(
            "Audio format value must be a number if present.",
            GRANDIOSE_INVALID_ARGS);

      c->status = napi_get_named_property(env, configValue, "referenceLevel", &param);
      REJECT_RETURN;
      c->status = napi_typeof(env, param, &type);
      REJECT_RETURN;
      if (type == napi_number)
      {
        c->status = napi_get_value_int32(env, param, &c->referenceLevel);
        REJECT_RETURN;
      }
      else if (type != napi_undefined)
        REJECT_ERROR_RETURN(
            "Audio reference level must be a number if present.",
            GRANDIOSE_INVALID_ARGS);
    }
    c->status = napi_typeof(env, waitValue, &type);
    REJECT_RETURN;
    if (type == napi_number)
    {
      c->status = napi_get_value_uint32(env, waitValue, &c->wait);
      REJECT_RETURN;
    }
  }

  napi_value resource_name;
  c->status = napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &resource_name);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resource_name, execute,
                                     complete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

napi_value audioReceive(napi_env env, napi_callback_info info)
{
  return dataAndAudioReceive(env, info, "AudioReceive",
                             audioReceiveExecute, audioReceiveComplete);
}

void metadataReceiveExecute(napi_env env, void *data)
{
  dataCarrier *c = (dataCarrier *)data;

  // printf("Metadata receiver executing.\n");

  switch (NDIlib_recv_capture_v2(c->recv, nullptr, nullptr, &c->metadataFrame, c->wait))
  {
  case NDIlib_frame_type_none:
    // printf("No data received.\n");
    c->status = GRANDIOSE_NOT_FOUND;
    c->errorMsg = "No audio data received in the requested time interval.";
    break;

  // Metadata
  case NDIlib_frame_type_metadata:
    break;

  default:
    // printf("Other kind of data received.\n");
    c->status = GRANDIOSE_NOT_AUDIO;
    c->errorMsg = "Non-metadata payload received on metadata capture.";
    break;
  }
}

void metadataReceiveComplete(napi_env env, napi_status asyncStatus, void *data)
{
  dataCarrier *c = (dataCarrier *)data;
  // printf("Metadata receiver completing - status %i.\n", c->status);

  if (asyncStatus != napi_ok)
  {
    c->status = asyncStatus;
    c->errorMsg = "Async metadata payload receive failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  napi_value param;
  c->status = napi_create_string_utf8(env, "metadata", NAPI_AUTO_LENGTH, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->metadataFrame.length, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "length", param);
  REJECT_STATUS;

  napi_value params, paramn;
  c->status = napi_create_int32(env, (int32_t)(c->metadataFrame.timecode / 10000000), &params);
  REJECT_STATUS;
  c->status = napi_create_int32(env, (c->metadataFrame.timecode % 10000000) * 100, &paramn);
  REJECT_STATUS;
  c->status = napi_create_array(env, &param);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 0, params);
  REJECT_STATUS;
  c->status = napi_set_element(env, param, 1, paramn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "timecode", param);
  REJECT_STATUS;

  c->status = napi_create_string_utf8(env, c->metadataFrame.p_data, NAPI_AUTO_LENGTH, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "data", param);
  REJECT_STATUS;

  NDIlib_recv_free_metadata(c->recv, &c->metadataFrame);

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value metadataReceive(napi_env env, napi_callback_info info)
{
  napi_valuetype type;
  dataCarrier *c = new dataCarrier;

  napi_value promise;
  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];
  napi_value thisValue;
  c->status = napi_get_cb_info(env, info, &argc, args, &thisValue, nullptr);
  REJECT_RETURN;

  napi_value recvValue;
  c->status = napi_get_named_property(env, thisValue, "embedded", &recvValue);
  REJECT_RETURN;
  void *recvData;
  c->status = napi_get_value_external(env, recvValue, &recvData);
  c->recv = (NDIlib_recv_instance_t)recvData;
  REJECT_RETURN;

  if (argc >= 1)
  {
    c->status = napi_typeof(env, args[0], &type);
    REJECT_RETURN;
    if (type == napi_number)
    {
      c->status = napi_get_value_uint32(env, args[0], &c->wait);
      REJECT_RETURN;
    }
  }

  napi_value resource_name;
  c->status = napi_create_string_utf8(env, "MetadataReceive", NAPI_AUTO_LENGTH, &resource_name);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resource_name, metadataReceiveExecute,
                                     metadataReceiveComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

void dataReceiveExecute(napi_env env, void *data)
{
  dataCarrier *c = (dataCarrier *)data;

  // printf("Audio receiver executing.\n");
  c->frameType = NDIlib_recv_capture_v2(c->recv, &c->videoFrame, &c->audioFrame, &c->metadataFrame, c->wait);
  switch (c->frameType)
  {

  // Audio data
  case NDIlib_frame_type_audio:
    switch (c->audioFormat)
    {
    case Grandiose_audio_format_int_16_interleaved:
      c->audioFrame16s.reference_level = c->referenceLevel;
      c->audioFrame16s.p_data = new short[c->audioFrame.no_samples * c->audioFrame.no_channels];
      NDIlib_util_audio_to_interleaved_16s_v2(&c->audioFrame, &c->audioFrame16s);
      break;
    case Grandiose_audio_format_float_32_interleaved:
      c->audioFrame32fIlvd.p_data = new float[c->audioFrame.no_samples * c->audioFrame.no_channels];
      NDIlib_util_audio_to_interleaved_32f_v2(&c->audioFrame, &c->audioFrame32fIlvd);
      break;
    case Grandiose_audio_format_float_32_separate:
    default:
      break;
    }
    break;

  // Handle all other types on completion
  default:
    break;
  }
}

void dataReceiveComplete(napi_env env, napi_status asyncStatus, void *data)
{
  dataCarrier *c = (dataCarrier *)data;

  if (asyncStatus != napi_ok)
  {
    c->status = asyncStatus;
    c->errorMsg = "Async data payload receive failed to complete.";
  }
  REJECT_STATUS;

  switch (c->frameType)
  {
  case NDIlib_frame_type_video:
    videoReceiveComplete(env, asyncStatus, data);
    break;
  case NDIlib_frame_type_audio:
    audioReceiveComplete(env, asyncStatus, data);
    break;
  case NDIlib_frame_type_metadata:
    metadataReceiveComplete(env, asyncStatus, data);
    break;
  case NDIlib_frame_type_error:
    c->errorMsg = "Received error response from NDI data request. Connection lost.";
    c->status = GRANDIOSE_CONNECTION_LOST;
    REJECT_STATUS;
  case NDIlib_frame_type_status_change:
    napi_value result, param;
    c->status = napi_create_object(env, &result);
    REJECT_STATUS;
    c->status = napi_create_string_utf8(env, "statusChange", NAPI_AUTO_LENGTH, &param);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "type", param);
    REJECT_STATUS;

    napi_status status;
    status = napi_resolve_deferred(env, c->_deferred, result);
    FLOATING_STATUS;

    tidyCarrier(env, c);
    break;
  }
}

napi_value dataReceive(napi_env env, napi_callback_info info)
{
  return dataAndAudioReceive(env, info, "DataReceive",
                             dataReceiveExecute, dataReceiveComplete);
}
