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

#include <stddef.h>
#include <cmath>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "grandiose_send.h"
#include "grandiose_util.h"

// Send audio frame
napi_value send(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value result;

  // Guard unable to initialize NDIlib
  if (!NDIlib_initialize())
    NAPI_THROW_ERROR("Failed to inittialise NDI subsystem.");

  // Create an NDI source that is called "My 16bpp Audio" and is clocked to the audio.
  NDIlib_send_create_t NDI_send_create_desc;
  NDI_send_create_desc.p_ndi_name = "My 16bpp Audio";
  NDI_send_create_desc.clock_audio = true;

  // We create the NDI finder
  NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
  if (!pNDI_send)
    NAPI_THROW_ERROR("Failed to create send instance.");

  // We are going to send 1920 audio samples at a time
  NDIlib_audio_frame_interleaved_16s_t NDI_audio_frame;
  NDI_audio_frame.sample_rate = 48000;
  NDI_audio_frame.no_channels = 2;
  NDI_audio_frame.no_samples = 1920;
  NDI_audio_frame.p_data = (short *)malloc(1920 * 2 * sizeof(short));

  for (int x = 0; x < 1920; x++)
  {
    short value = (short)(sin((x / 240.0) * 3.1415 * 2.0) * 28000);
    // printf("Next value %i.\n", value);
    NDI_audio_frame.p_data[x * 2] = value;
    NDI_audio_frame.p_data[x * 2 + 1] = value;
  }
  // We will send 1000 frames of audio.
  for (int idx = 0; idx < 1000; idx++)
  {
    // Fill in the buffer with silence. It is likely that you would do something much smarter than this.
    //memset(NDI_audio_frame.p_data, 0, NDI_audio_frame.no_samples*NDI_audio_frame.no_channels*sizeof(short));

    // We now submit the frame. Note that this call will be clocked so that we end up submitting
    // at exactly 48kHz
    NDIlib_util_send_send_audio_interleaved_16s(pNDI_send, &NDI_audio_frame);

    // Just display something helpful
    // printf("Frame number %d sent.\n", idx);
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
