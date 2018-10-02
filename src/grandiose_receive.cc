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
#include <Processing.NDI.Lib.h>
#include <inttypes.h>

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

void finalizeReceive(napi_env env, void* data, void* hint) {
  printf("Releasing receiver.\n");
  NDIlib_recv_destroy((NDIlib_recv_instance_t) data);
}

void receiveExecute(napi_env env, void* data) {
  receiveCarrier* c = (receiveCarrier*) data;

  NDIlib_recv_create_v3_t receiveConfig;
  receiveConfig.source_to_connect_to = *c->source;
  receiveConfig.color_format = c->colorFormat;
  receiveConfig.bandwidth = c->bandwidth;
  receiveConfig.allow_video_fields = c->allowVideoFields;
  receiveConfig.p_ndi_name = c->name;

  c->recv = NDIlib_recv_create_v3(&receiveConfig);
  if (!c->recv) {
    c->status = GRANDIOSE_RECEIVE_CREATE_FAIL;
    c->errorMsg = "Failed to create NDI receiver.";
    return;
  }

  NDIlib_recv_connect(c->recv, c->source);
}

void receiveComplete(napi_env env, napi_status asyncStatus, void* data) {
  receiveCarrier* c = (receiveCarrier*) data;

  printf("Completing some receive creation work.\n");

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async receiver creation failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  napi_value embedded;
  c->status = napi_create_external(env, c->recv, finalizeReceive, nullptr, &embedded);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "embedded", embedded);
  REJECT_STATUS;

  napi_value videoFn;
  c->status = napi_create_function(env, "video", NAPI_AUTO_LENGTH, videoReceive,
    nullptr, &videoFn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "video", videoFn);
  REJECT_STATUS;

  napi_value audioFn;
  c->status = napi_create_function(env, "audio", NAPI_AUTO_LENGTH, audioReceive,
    nullptr, &audioFn);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "audio", audioFn);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value receive(napi_env env, napi_callback_info info) {
  napi_valuetype type;
  receiveCarrier* c = new receiveCarrier;

  napi_value promise;
  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];
  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  if (argc != (size_t) 1) {
    c->errorMsg = "Receiver must be created with an object containing at least a 'source' property.";
    c->status = GRANDIOSE_INVALID_ARGS;
    REJECT_RETURN;
  }

  c->status = napi_typeof(env, args[0], &type);
  REJECT_RETURN;
  bool isArray;
  c->status = napi_is_array(env, args[0], &isArray);
  REJECT_RETURN;
  if ((type != napi_object) || isArray) {
    c->errorMsg = "Single argument must be an object, not an array, containing at least a 'source' property.";
    c->status = GRANDIOSE_INVALID_ARGS;
    REJECT_RETURN;
  }

  napi_value config = args[0];
  napi_value source, colorFormat, bandwidth, allowVideoFields, name;
  // source is an object, not an array, with name and urlAddress
  // convert to a native source
  c->status = napi_get_named_property(env, config, "source", &source);
  REJECT_RETURN;
  c->status = napi_typeof(env, source, &type);
  REJECT_RETURN;
  c->status = napi_is_array(env, source, &isArray);
  REJECT_RETURN;
  if ((type != napi_object) || isArray) {
    c->errorMsg = "Source property must be an object and not an array.";
    c->status = GRANDIOSE_INVALID_ARGS;
    REJECT_RETURN;
  }

  napi_value checkType;
  c->status = napi_get_named_property(env, source, "name", &checkType);
  REJECT_RETURN;
  c->status = napi_typeof(env, checkType, &type);
  REJECT_RETURN;
  if (type != napi_string) {
    c->errorMsg = "Source property must have a 'name' sub-property that is of type string.";
    c->status = GRANDIOSE_INVALID_ARGS;
    REJECT_RETURN;
  }

  c->status = napi_get_named_property(env, source, "urlAddress", &checkType);
  REJECT_RETURN;
  c->status = napi_typeof(env, checkType, &type);
  REJECT_RETURN;
  if (type != napi_string) {
    c->errorMsg = "Source property must have a 'urlAddress' sub-property that is of type string.";
    c->status = GRANDIOSE_INVALID_ARGS;
    REJECT_RETURN;
  }

  c->source = new NDIlib_source_t();
  c->status = makeNativeSource(env, source, c->source);
  REJECT_RETURN;

  c->status = napi_get_named_property(env, config, "colorFormat", &colorFormat);
  REJECT_RETURN;
  c->status = napi_typeof(env, colorFormat, &type);
  REJECT_RETURN;
  if (type != napi_undefined) {
    if (type != napi_number) {
      c->errorMsg = "Color format property must be a number.";
      c->status = GRANDIOSE_INVALID_ARGS;
      REJECT_RETURN;
    }
    int32_t enumValue;
    c->status = napi_get_value_int32(env, colorFormat, &enumValue);
    REJECT_RETURN;

    c->colorFormat = (NDIlib_recv_color_format_e) enumValue;
    if (!validColorFormat(c->colorFormat)) {
      c->errorMsg = "Invalid colour format value.";
      c->status = GRANDIOSE_INVALID_ARGS;
      REJECT_RETURN;
    }
  }

  c->status = napi_get_named_property(env, config, "bandwidth", &bandwidth);
  REJECT_RETURN;
  c->status = napi_typeof(env, bandwidth, &type);
  REJECT_RETURN;
  if (type != napi_undefined) {
    if (type != napi_number) {
      c->errorMsg = "Bandwidth property must be a number.";
      c->status = GRANDIOSE_INVALID_ARGS;
      REJECT_RETURN;
    }
    int32_t enumValue;
    c->status = napi_get_value_int32(env, bandwidth, &enumValue);
    REJECT_RETURN;

    c->bandwidth = (NDIlib_recv_bandwidth_e) enumValue;
    if (!validBandwidth(c->bandwidth)) {
      c->errorMsg = "Invalid bandwidth value.";
      c->status = GRANDIOSE_INVALID_ARGS;
      REJECT_RETURN;
    }
  }

  c->status = napi_get_named_property(env, config, "allowVideoFields", &allowVideoFields);
  REJECT_RETURN;
  c->status = napi_typeof(env, allowVideoFields, &type);
  REJECT_RETURN;
  if (type != napi_undefined) {
    if (type != napi_boolean) {
      c->errorMsg = "Allow video fields property must be a Boolean.";
      c->status = GRANDIOSE_INVALID_ARGS;
      REJECT_RETURN;
    }
    c->status = napi_get_value_bool(env, allowVideoFields, &c->allowVideoFields);
    REJECT_RETURN;
  }

  c->status = napi_get_named_property(env, config, "name", &name);
  REJECT_RETURN;
  c->status = napi_typeof(env, name, &type);
  if (type != napi_undefined) {
    if (type != napi_string) {
      c->errorMsg = "Optional name property must be a string when present.";
      c->status = GRANDIOSE_INVALID_ARGS;
      REJECT_RETURN;
    }
    size_t namel;
    c->status = napi_get_value_string_utf8(env, name, nullptr, 0, &namel);
    REJECT_RETURN;
    c->name = (char *) malloc(namel + 1);
    c->status = napi_get_value_string_utf8(env, name, c->name, namel + 1, &namel);
    REJECT_RETURN;
  }

  napi_value resource_name;
  c->status = napi_create_string_utf8(env, "Receive", NAPI_AUTO_LENGTH, &resource_name);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resource_name, receiveExecute,
    receiveComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

void videoReceiveExecute(napi_env env, void* data) {
  videoCarrier* c = (videoCarrier*) data;

  switch (NDIlib_recv_capture_v2(c->recv, &c->videoFrame, nullptr, nullptr, c->wait))
  {
    case NDIlib_frame_type_none:
      printf("No data received.\n");
      c->status = GRANDIOSE_NOT_FOUND;
      c->errorMsg = "No video data received in the requested time interval.";
      break;

    // Video data
    case NDIlib_frame_type_video:
      /* printf("Video data %i received (%dx%d at %d/%d).\n", &c->videoFrame, c->videoFrame.xres, c->videoFrame.yres,
        c->videoFrame.frame_rate_N, c->videoFrame.frame_rate_D); */
      break;

    default:
      printf("Other kind of data received.\n");
      c->status = GRANDIOSE_NOT_VIDEO;
      c->errorMsg = "Non-video data received on video capture.";
      break;
  }
}

void videoReceiveComplete(napi_env env, napi_status asyncStatus, void* data) {
  videoCarrier* c = (videoCarrier*) data;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async video frame receive failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  int32_t ptps, ptpn;
  ptps = (int32_t) (c->videoFrame.timestamp / 10000000);
  ptpn = (c->videoFrame.timestamp % 10000000) * 100;

  napi_value param;
  c->status = napi_create_string_utf8(env, "video", NAPI_AUTO_LENGTH, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->videoFrame.xres, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "xres", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->videoFrame.yres, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "yres", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->videoFrame.frame_rate_N, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frameRateN", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->videoFrame.frame_rate_D, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frameRateD", param);
  REJECT_STATUS;

  c->status = napi_create_double(env, (double) c->videoFrame.picture_aspect_ratio, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "pictureAspectRatio", param);
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

  c->status = napi_create_int32(env, c->videoFrame.frame_format_type, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frameFormatType", param);
  REJECT_STATUS;

  c->status = napi_create_int32(env, (int32_t) c->videoFrame.timecode / 10000000, &params);
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

  c->status = napi_create_int32(env, c->videoFrame.line_stride_in_bytes, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "lineStrideBytes", param);
  REJECT_STATUS;

  if (c->videoFrame.p_metadata != nullptr) {
    c->status = napi_create_string_utf8(env, c->videoFrame.p_metadata, NAPI_AUTO_LENGTH, &param);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "metadata", param);
    REJECT_STATUS;
  }

  c->status = napi_create_buffer_copy(env,
    c->videoFrame.line_stride_in_bytes * c->videoFrame.yres,
    (void*) c->videoFrame.p_data, nullptr, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "data", param);
  REJECT_STATUS;

  NDIlib_recv_free_video_v2(c->recv, &c->videoFrame);

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value videoReceive(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_valuetype type;
  videoCarrier* carrier = new videoCarrier;

  size_t argc = 1;
  napi_value args[1];
  napi_value thisValue;
  status = napi_get_cb_info(env, info, &argc, args, &thisValue, nullptr);
  CHECK_STATUS;

  napi_value recvValue;
  status = napi_get_named_property(env, thisValue, "embedded", &recvValue);
  CHECK_STATUS;
  void* recvData;
  status = napi_get_value_external(env, recvValue, &recvData);
  carrier->recv = (NDIlib_recv_instance_t) recvData;
  CHECK_STATUS;

  if (argc >= 1) {
    status = napi_typeof(env, args[0], &type);
    CHECK_STATUS;
    if (type == napi_number) {
      status = napi_get_value_uint32(env, args[0], &carrier->wait);
      CHECK_STATUS;
    }
  }

  napi_value promise;
  status = napi_create_promise(env, &carrier->_deferred, &promise);
  CHECK_STATUS;

  napi_value resource_name;
  status = napi_create_string_utf8(env, "VideoReceive", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, videoReceiveExecute,
    videoReceiveComplete, carrier, &carrier->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, carrier->_request);
  CHECK_STATUS;

  return promise;
}

void audioReceiveExecute(napi_env env, void* data) {
  audioCarrier* c = (audioCarrier*) data;

  printf("Audio receiver executing.\n");

  switch (NDIlib_recv_capture_v2(c->recv, nullptr, &c->audioFrame, nullptr, c->wait))
  {
    case NDIlib_frame_type_none:
      printf("No data received.\n");
      c->status = GRANDIOSE_NOT_FOUND;
      c->errorMsg = "No audio data received in the requested time interval.";
      break;

    // Video data
    case NDIlib_frame_type_audio:
      /* printf("Video data %i received (%dx%d at %d/%d).\n", &c->videoFrame, c->videoFrame.xres, c->videoFrame.yres,
        c->videoFrame.frame_rate_N, c->videoFrame.frame_rate_D); */
      break;

    default:
      printf("Other kind of data received.\n");
      c->status = GRANDIOSE_NOT_AUDIO;
      c->errorMsg = "Non-video data received on video capture.";
      break;
  }
}

void audioReceiveComplete(napi_env env, napi_status asyncStatus, void* data) {
  audioCarrier* c = (audioCarrier*) data;

  printf("Audio receiver completing - status %i.\n", c->status);

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async audio frame receive failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  int32_t ptps, ptpn;
  ptps = (int32_t) (c->audioFrame.timestamp / 10000000);
  ptpn = (c->audioFrame.timestamp % 10000000) * 100;

  napi_value param;
  c->status = napi_create_string_utf8(env, "audio", NAPI_AUTO_LENGTH, &param);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", param);
  REJECT_STATUS;

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

  c->status = napi_create_int32(env, c->audioFrame.channel_stride_in_bytes, &param);
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
  c->status = napi_create_int32(env, (int32_t) (c->audioFrame.timecode / 10000000), &params);
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

  if (c->audioFrame.p_metadata != nullptr) {
    c->status = napi_create_string_utf8(env, c->audioFrame.p_metadata, NAPI_AUTO_LENGTH, &param);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "metadata", param);
    REJECT_STATUS;
  }

  char * rawFloats = (char*) c->audioFrame.p_data;
  // printf("Float %f, raw1 = %i, raw2 = %i, raw3 = %i, raw4 = %i", c->audioFrame.p_data[0],
  //  rawFloats[0], rawFloats[1], rawFloats[2], rawFloats[3]);
  c->status = napi_create_buffer_copy(env,
    c->audioFrame.channel_stride_in_bytes * c->audioFrame.no_channels,
    rawFloats, nullptr, &param);
  REJECT_STATUS;
  /* napi_value paramt;
  c->status = napi_create_typedarray(env, napi_float32_array,
    c->audioFrame.no_samples * c->audioFrame.no_channels, param, 0, &paramt);
  const napi_extended_error_info* errorInfo = (const napi_extended_error_info*) malloc(sizeof(napi_extended_error_info));
  napi_get_last_error_info(env, &errorInfo);
  printf("Status is %i with error %s.\n", c->status, errorInfo->error_message);
  c->errorMsg = errorInfo->error_message;
  REJECT_STATUS; */
  c->status = napi_set_named_property(env, result, "data", param);
  REJECT_STATUS;

  NDIlib_recv_free_audio_v2(c->recv, &c->audioFrame);

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value audioReceive(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_valuetype type;
  audioCarrier* carrier = new audioCarrier;

  size_t argc = 1;
  napi_value args[1];
  napi_value thisValue;
  status = napi_get_cb_info(env, info, &argc, args, &thisValue, nullptr);
  CHECK_STATUS;

  napi_value recvValue;
  status = napi_get_named_property(env, thisValue, "embedded", &recvValue);
  CHECK_STATUS;
  void* recvData;
  status = napi_get_value_external(env, recvValue, &recvData);
  carrier->recv = (NDIlib_recv_instance_t) recvData;
  CHECK_STATUS;

  if (argc >= 1) {
    status = napi_typeof(env, args[0], &type);
    CHECK_STATUS;
    if (type == napi_number) {
      status = napi_get_value_uint32(env, args[0], &carrier->wait);
      CHECK_STATUS;
    }
  }

  napi_value promise;
  status = napi_create_promise(env, &carrier->_deferred, &promise);
  CHECK_STATUS;

  napi_value resource_name;
  status = napi_create_string_utf8(env, "audioReceive", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, audioReceiveExecute,
    audioReceiveComplete, carrier, &carrier->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, carrier->_request);
  CHECK_STATUS;

  return promise;
}

void metadataReceiveExecute(napi_env env, void* data) {

}

void metadataReceiveComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value metadataReceive(napi_env env, napi_callback_info info) {

}

void dataReceiveExecute(napi_env env, void* data) {

}

void dataReceiveComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value dataReceive(napi_env env, napi_callback_info info) {

}

/* napi_value receive_old(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;

  if (!NDIlib_initialize()) return 0;

	// Create a finder
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2();
	if (!pNDI_find) return 0;

	// Wait until there is one source
	uint32_t no_sources = 0;
	const NDIlib_source_t* p_sources = NULL;
  int count = 0;
	while ((!no_sources) || (count++ < 10))
	{	// Wait until the sources on the nwtork have changed
		printf("Looking for sources ...\n");
		NDIlib_find_wait_for_sources(pNDI_find, 5000);
		p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
    printf("Found %u sources, first is %s.\n", no_sources, p_sources[0].p_ndi_name);
	}

  printf("Found %u sources, first is %s.\n", no_sources, p_sources[0].p_ndi_name);

	// We now have at least one source, so we create a receiver to look at it.
	NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3();
	if (!pNDI_recv) return 0;

  printf("Got receive instance.\n");
	// Connect to our sources
	NDIlib_recv_connect(pNDI_recv, p_sources + 0);

  printf("Connected to receive instance.\n");
	// Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
	NDIlib_find_destroy(pNDI_find);

  printf("Killed the finder. Starting capture.\n");
	// Run for one minute
	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < seconds(10);)
	{	// The descriptors
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_audio_frame_v2_t audio_frame;

		switch (NDIlib_recv_capture_v2(pNDI_recv, &video_frame, &audio_frame, nullptr, 5000))
		{	// No data
			case NDIlib_frame_type_none:
				printf("No data received.\n");
				break;

			// Video data
			case NDIlib_frame_type_video:
				printf("Video data received (%dx%d at %d/%d).\n", video_frame.xres, video_frame.yres,
          video_frame.frame_rate_N, video_frame.frame_rate_D);
				NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
				break;

			// Audio data
			case NDIlib_frame_type_audio:
				printf("Audio data received (%d samples).\n", audio_frame.no_samples);
				NDIlib_recv_free_audio_v2(pNDI_recv, &audio_frame);
				break;
		}
	}

	// Destroy the receiver
	NDIlib_recv_destroy(pNDI_recv);

	// Not required, but nice
	NDIlib_destroy();


  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  return result;
} */
