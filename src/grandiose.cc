#include <cstdio>
#include <chrono>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "grandiose_util.h"
#include "node_api.h"

napi_value version(napi_env env, napi_callback_info info) {
  napi_status status;

  const char* ndiVersion = NDIlib_version();
  napi_value result;
  status = napi_create_string_utf8(env, ndiVersion, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value isSupportedCPU(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value result;
  status = napi_get_boolean(env, NDIlib_is_supported_CPU(), &result);
  CHECK_STATUS;

  return result;
}

// MDNS does not appear to be working within Node, OK (as MDNS ever is) autside
napi_value find(napi_env env, napi_callback_info info) {
  napi_status status;

  if (!NDIlib_initialize()) NAPI_THROW_ERROR("Failed to inittialise NDI subsystem.");

  NDIlib_find_create_t find_create;
  find_create.show_local_sources = true;
  find_create.p_groups = nullptr;
  find_create.p_extra_ips = nullptr;

  size_t argc = 1;
  napi_value argv[1];
  status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  CHECK_STATUS;
  if (argc >= 1) {
    napi_valuetype type;
    status = napi_typeof(env, argv[0], &type);
    CHECK_STATUS;
    if (type == napi_object) {
      napi_value property;
      status = napi_get_named_property(env, argv[0], "showLocalSources", &property);
      if (status == napi_ok) {
        status = napi_typeof(env, property, &type);
        CHECK_STATUS;
        if (type == napi_boolean) {
          status = napi_get_value_bool(env, property, &find_create.show_local_sources);
          CHECK_STATUS;
          printf("Find create show local sources %i\n", find_create.show_local_sources);
        }
      } // status == napi_ok for showLocalSources
      status = napi_get_named_property(env, argv[0], "groups", &property);
      if (status == napi_ok) {
        status = napi_typeof(env, property, &type);
        CHECK_STATUS;
        switch (type) {
          case napi_string:
            size_t len;
            status = napi_get_value_string_utf8(env, property, nullptr, 0, &len);
            CHECK_STATUS;
            find_create.p_groups = (const char *) malloc(len + 1);
            status = napi_get_value_string_utf8(env, property,
              (char *) find_create.p_groups, len + 1, &len);
            CHECK_STATUS;
            printf("Find create groups %s\n", find_create.p_groups);
            break;
          default:
            break;
        }
      } // status == napi_ok for p_groups
      status = napi_get_named_property(env, argv[0], "extraIPs", &property);
      if (status == napi_ok) {
        status = napi_typeof(env, property, &type);
        CHECK_STATUS;
        switch (type) {
          case napi_string:
            size_t len;
            status =  napi_get_value_string_utf8(env, property, nullptr, 0, &len);
            CHECK_STATUS;
            find_create.p_extra_ips = (const char *) malloc(len + 1);
            status = napi_get_value_string_utf8(env, property,
              (char *) find_create.p_extra_ips, len + 1, &len);
            CHECK_STATUS;
            printf("Find create groups %s\n", find_create.p_extra_ips);
            break;
          default:
            break;
        }
      }

    } // type == napi_object
  } // argc >= 1

	// We are going to create an NDI finder that locates sources on the network.
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2(&find_create);
	if (!pNDI_find) NAPI_THROW_ERROR("Failed to create NDI find instance.");

	uint32_t no_sources = 0;
//  uint32_t waiting = NDIlib_find_wait_for_sources(pNDI_find, 5000);
//   printf("Waiting %u\n", waiting);
	const NDIlib_source_t* p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);

  printf("Sources: %u\n", no_sources);

  napi_value result;
  status = napi_create_array(env, &result);
  CHECK_STATUS;

  for ( uint32_t i = 0 ; i < no_sources; i++ ) {
    napi_value name, uri, item;
    status = napi_create_string_utf8(env, p_sources[i].p_ndi_name, NAPI_AUTO_LENGTH, &name);
    CHECK_STATUS;
    status = napi_create_string_utf8(env, p_sources[i].p_url_address, NAPI_AUTO_LENGTH, &uri);
    CHECK_STATUS;
    status = napi_create_object(env, &item);
    CHECK_STATUS;
    status = napi_set_named_property(env, item, "name", name);
    CHECK_STATUS;
    status = napi_set_named_property(env, item, "urlAddress", uri);
    CHECK_STATUS;

    status = napi_set_element(env, result, i, item);
    CHECK_STATUS;
  }

	// Destroy the NDI finder
	NDIlib_find_destroy(pNDI_find);

	// Finished
	NDIlib_destroy();

  return result;
}

napi_value send(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;

  if (!NDIlib_initialize()) NAPI_THROW_ERROR("Failed to inittialise NDI subsystem.");

  // Create an NDI source that is called "My 16bpp Audio" and is clocked to the audio.
  NDIlib_send_create_t NDI_send_create_desc;
  NDI_send_create_desc.p_ndi_name = "My 16bpp Audio";
  NDI_send_create_desc.clock_audio = true;

  // We create the NDI finder
  NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
  if (!pNDI_send) NAPI_THROW_ERROR("Failed to create send instance.");

  // We are going to send 1920 audio samples at a time
  NDIlib_audio_frame_interleaved_16s_t NDI_audio_frame;
  NDI_audio_frame.sample_rate = 48000;
  NDI_audio_frame.no_channels = 2;
  NDI_audio_frame.no_samples = 1920;
  NDI_audio_frame.p_data = (short*)malloc(1920 * 2 * sizeof(short));

  for ( int x = 0 ; x < 1920 ; x++ ) {
    short value = (short) (sin((x / 240.0) * 3.1415 * 2.0) * 28000);
    // printf("Next value %i.\n", value);
    NDI_audio_frame.p_data[x * 2] = value;
    NDI_audio_frame.p_data[x * 2 + 1] = value;
  }
  // We will send 1000 frames of audio.
  for ( int idx = 0 ; idx < 1000 ; idx++ )
  {	// Fill in the buffer with silence. It is likely that you would do something much smarter than this.
    //memset(NDI_audio_frame.p_data, 0, NDI_audio_frame.no_samples*NDI_audio_frame.no_channels*sizeof(short));

    // We now submit the frame. Note that this call will be clocked so that we end up submitting
    // at exactly 48kHz
    NDIlib_util_send_send_audio_interleaved_16s(pNDI_send, &NDI_audio_frame);

    // Just display something helpful
    printf("Frame number %d sent.\n", idx);
  }

  // Free the video frame
  free(NDI_audio_frame.p_data);

  // Destroy the NDI finder
  NDIlib_send_destroy(pNDI_send);

  // Not required, but nice
  NDIlib_destroy();

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  return result;
}

napi_value receive(napi_env env, napi_callback_info info) {
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
		NDIlib_find_wait_for_sources(pNDI_find, 5000/* One second */);
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
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("version", version),
    DECLARE_NAPI_METHOD("find", find),
    DECLARE_NAPI_METHOD("isSupportedCPU", isSupportedCPU),
    DECLARE_NAPI_METHOD("send", send),
    DECLARE_NAPI_METHOD("receive", receive)
   };
  status = napi_define_properties(env, exports, 5, desc);
  CHECK_STATUS;

  return exports;
}

NAPI_MODULE(nodencl, Init)
