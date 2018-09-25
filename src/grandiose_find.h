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

#ifndef GRANDIOSE_FIND_H
#define GRANDIOSE_FIND_H

#include "node_api.h"
#include "grandiose_util.h"

napi_value find(napi_env env, napi_callback_info info);

struct findCarrier : carrier {
  uint32_t wait = 10000;
  NDIlib_find_instance_t find;
  uint32_t no_sources = 0;
  const NDIlib_source_t* sources;
};

#define GRANDIOSE_NOT_FOUND 404

#endif /* GRANDIOSE_FIND_H */
