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

#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "grandiose_util.h"
#include "grandiose_find.h"

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
