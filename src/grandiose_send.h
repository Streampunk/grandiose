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

#ifndef GRANDIOSE_SEND_H
#define GRANDIOSE_SEND_H

#include "node_api.h"
#include "grandiose_util.h"

napi_value send(napi_env env, napi_callback_info info);

struct sendCarrier : carrier {
  char* name = nullptr;
  char* groups = nullptr;
  bool clockVideo = false;
  bool clockAudio = false;
  NDIlib_send_instance_t send;
  ~sendCarrier() {
    free(name);
  }
};

struct sendDataCarrier : carrier {
  NDIlib_send_instance_t send;
  NDIlib_video_frame_v2_t videoFrame;
  NDIlib_audio_frame_v2_t audioFrame;
  NDIlib_metadata_frame_t metadataFrame;
  napi_ref sourceBufferRef = nullptr;
  ~sendDataCarrier() {
    // TODO: free sourceBufferRef
  }
};


#endif /* GRANDIOSE_SEND_H */
