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

void finalizeVideo(napi_env env, void* data, void* hint) {
  printf("Releasing video frame.\n");
  NDIlib_recv_free_video_v2((NDIlib_recv_instance_t) hint, (NDIlib_video_frame_v2_t*) data);
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

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value receive(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_valuetype type;
  receiveCarrier* carrier = new receiveCarrier;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  printf("I've got %i args.\n", argc);

  if (argc != (size_t) 1) NAPI_THROW_ERROR("Receiver must be created with an object containing at least a 'source' property.");

  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  bool isArray;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if ((type != napi_object) || isArray)
    NAPI_THROW_ERROR("Single argument must be an object, not an array, containing at least a 'source' property.");

  napi_value config = args[0];
  napi_value source, colorFormat, bandwidth, allowVideoFields, name;
  // source is an object, not an array, with name and urlAddress
  // convert to a native source
  status = napi_get_named_property(env, config, "source", &source);
  printf("Status is %i.\n", status);
  CHECK_STATUS;
  status = napi_typeof(env, source, &type);
  CHECK_STATUS;
  status = napi_is_array(env, source, &isArray);
  CHECK_STATUS;
  if ((type != napi_object) || isArray) NAPI_THROW_ERROR("Source property must be an object and not an array.");

  napi_value checkType;
  status = napi_get_named_property(env, source, "name", &checkType);
  CHECK_STATUS;
  status = napi_typeof(env, checkType, &type);
  CHECK_STATUS;
  if (type != napi_string) NAPI_THROW_ERROR("Source property must have a 'name' sub-property that is of type string.");
  status = napi_get_named_property(env, source, "urlAddress", &checkType);
  CHECK_STATUS;
  status = napi_typeof(env, checkType, &type);
  CHECK_STATUS;
  if (type != napi_string) NAPI_THROW_ERROR("Source property must have a 'urlAddress' sub-property that is of type string.");

  carrier->source = new NDIlib_source_t();
  status = makeNativeSource(env, source, carrier->source);
  CHECK_STATUS;

  status = napi_get_named_property(env, config, "colorFormat", &colorFormat);
  CHECK_STATUS;
  status = napi_typeof(env, colorFormat, &type);
  CHECK_STATUS;
  if (type != napi_undefined) {
    if (type != napi_number) NAPI_THROW_ERROR("Color format property must be a number.");
    int32_t enumValue;
    status = napi_get_value_int32(env, colorFormat, &enumValue);
    CHECK_STATUS;

    carrier->colorFormat = (NDIlib_recv_color_format_e) enumValue;
    if (!validColorFormat(carrier->colorFormat)) NAPI_THROW_ERROR("Invalid colour format value.");
  }

  status = napi_get_named_property(env, config, "bandwidth", &bandwidth);
  CHECK_STATUS;
  status = napi_typeof(env, bandwidth, &type);
  CHECK_STATUS;
  if (type != napi_undefined) {
    if (type != napi_number) NAPI_THROW_ERROR("Bandwidth property must be a number.");
    int32_t enumValue;
    status = napi_get_value_int32(env, bandwidth, &enumValue);
    CHECK_STATUS;

    carrier->bandwidth = (NDIlib_recv_bandwidth_e) enumValue;
    if (!validBandwidth(carrier->bandwidth)) NAPI_THROW_ERROR("Invalid bandwidth value.");
  }

  status = napi_get_named_property(env, config, "allowVideoFields", &allowVideoFields);
  CHECK_STATUS;
  status = napi_typeof(env, allowVideoFields, &type);
  CHECK_STATUS;
  if (type != napi_undefined) {
    if (type != napi_boolean) NAPI_THROW_ERROR("Allow video fields property must be a Boolean.");
    status = napi_get_value_bool(env, allowVideoFields, &carrier->allowVideoFields);
    CHECK_STATUS;
  }

  status = napi_get_named_property(env, config, "name", &name);
  CHECK_STATUS;
  status = napi_typeof(env, name, &type);
  if (type != napi_undefined) {
    if (type != napi_string) NAPI_THROW_ERROR("Optional name property must be a string when present.");
    size_t namel;
    status = napi_get_value_string_utf8(env, name, nullptr, 0, &namel);
    CHECK_STATUS;
    carrier->name = (char *) malloc(namel + 1);
    status = napi_get_value_string_utf8(env, name, carrier->name, namel + 1, &namel);
    CHECK_STATUS;
  }

  napi_value promise;
  status = napi_create_promise(env, &carrier->_deferred, &promise);
  CHECK_STATUS;

  napi_value resource_name;
  status = napi_create_string_utf8(env, "Receive", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, receiveExecute,
    receiveComplete, carrier, &carrier->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, carrier->_request);
  CHECK_STATUS;

  return promise;
}

void videoReceiveExecute(napi_env env, void* data) {
  videoCarrier* c = (videoCarrier*) data;

  switch (NDIlib_recv_capture_v2(c->recv, &c->videoFrame, nullptr, nullptr, c->wait))
  {
    case NDIlib_frame_type_none:
      printf("No data received.\n");
      break;

    // Video data
    case NDIlib_frame_type_video:
      printf("Video data received (%dx%d at %d/%d).\n", c->videoFrame.xres, c->videoFrame.yres,
        c->videoFrame.frame_rate_N, c->videoFrame.frame_rate_D);
      break;

    default:
      printf("Other kind of data received.\n");
      break;
  }

}

void videoReceiveComplete(napi_env env, napi_status asyncStatus, void* data) {
  videoCarrier* c = (videoCarrier*) data;

  printf("Video receive completed.\n");

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async video frame receive failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  napi_value embedded;
  c->status = napi_create_external(env, &c->videoFrame, finalizeVideo, c->recv, &embedded);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "embedded", embedded);
  REJECT_STATUS;

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

}

void audioReceiveComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value audioReceive(napi_env env, napi_callback_info info) {

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
