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

#ifndef GRANDIOSE_RECEIVE_H
#define GRANDIOSE_RECEIVE_H

#include "node_api.h"
#include "grandiose_util.h"

napi_value receive(napi_env env, napi_callback_info info);
napi_value videoReceive(napi_env env, napi_callback_info info);
napi_value audioReceive(napi_env env, napi_callback_info info);
napi_value metadataReceive(napi_env env, napi_callback_info info);
napi_value dataReceive(napi_env env, napi_callback_info info);

struct receiveCarrier : carrier {
  NDIlib_source_t* source = nullptr;
  NDIlib_recv_color_format_e colorFormat = NDIlib_recv_color_format_fastest;
  NDIlib_recv_bandwidth_e bandwidth = NDIlib_recv_bandwidth_highest;
  bool allowVideoFields = true;
  char* name = nullptr;
  NDIlib_recv_instance_t recv;
  ~receiveCarrier() {
    free(name);
    if (source != nullptr) {
      delete source;
    }
  }
};

struct dataCarrier : carrier {
  uint32_t wait = 10000;
  NDIlib_recv_instance_t recv;
  NDIlib_frame_type_e frameType;
  NDIlib_video_frame_v2_t videoFrame;
  NDIlib_audio_frame_v2_t audioFrame;
  NDIlib_audio_frame_interleaved_16s_t audioFrame16s;
  NDIlib_audio_frame_interleaved_32f_t audioFrame32fIlvd;
  int32_t referenceLevel = 20;
  Grandiose_audio_format_e audioFormat = Grandiose_audio_format_float_32_separate;
  NDIlib_metadata_frame_t metadataFrame;
  ~dataCarrier() {
    delete[] audioFrame16s.p_data;
    delete[] audioFrame32fIlvd.p_data;
  }
};

#endif /* GRANDIOSE_RECEIVE_H */
