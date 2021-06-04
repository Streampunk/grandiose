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

#include <cmath>
#include <stddef.h>
#include <string.h>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "grandiose_send_video.h"
#include "grandiose_util.h"

// Send video frame
napi_value sendVideo(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value result;

  if (!NDIlib_initialize())
    NAPI_THROW_ERROR("Failed to inittialise NDI subsystem.");

  // Create an NDI source that is called "My 16bpp Audio" and is clocked to the audio.
  NDIlib_send_create_t NDI_send_create_desc;

  // Set name of NDI source
  NDI_send_create_desc.p_ndi_name = "My 8bpp video";
  NDI_send_create_desc.clock_audio = true;

  // We create the NDI finder
  NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
  if (!pNDI_send)
    NAPI_THROW_ERROR("Failed to create send instance.");

  // We are going to create a frame with following specs
  NDIlib_video_frame_v2_t NDI_video_frame;

  // - Resolution: 1920x1080
  NDI_video_frame.xres = 1920;
  NDI_video_frame.yres = 1080;

  // - Frame rate: 25Hz (25000/1000)
  NDI_video_frame.frame_rate_N = 25000;
  NDI_video_frame.frame_rate_D = 1000;

  // Data delivered in color format: BGRX
  NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRX;

  // Calculate number of pixels in video frame
  const auto number_of_pixels = NDI_video_frame.xres * NDI_video_frame.yres;

  // Allocate buffer for the video frame
  // 8 bit per channel (BGRX) per pixel
  NDI_video_frame.p_data = (uint8_t *)malloc(number_of_pixels * 4);

  // Run for 5 minutes
  using namespace std::chrono;
  for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5);)
  {
    // Get the current time
    const auto start_send = high_resolution_clock::now();

    // Send 200 frames
    for (int frame_index = 200; frame_index; frame_index--)
    {
      // Guard no connected receivers - timeout at 1000 ms
      if (!NDIlib_send_get_no_connections(pNDI_send, 1000))
      {
        printf("No current connections, so no rendering needed for frame %d.\n", frame_index);
        continue;
      }

      // Fill in the buffer. It is likely that you would do something much smarter than this.
      // All pixels is painted the same color - the color is switching between black and white based on the frame number
      memset((void *)NDI_video_frame.p_data, (frame_index & 1) ? 255 : 0, number_of_pixels * 4);

      // We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
      NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);
    }
    // Finish looping 200 frames

    // Display framerate for generated framerate
    printf("200 frames sent, at %1.2ffps\n", 200.0f / duration_cast<duration<float>>(high_resolution_clock::now() - start_send).count());
  }
  // Finish looping for desired number of minutes

  // Free the video frame
  free(NDI_video_frame.p_data);

  // Destroy the NDI sender
  NDIlib_send_destroy(pNDI_send);

  // Not required, but nice
  NDIlib_destroy();

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  return result;
}
