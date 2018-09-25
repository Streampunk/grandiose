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
