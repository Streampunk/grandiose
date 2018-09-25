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

void findExecute(napi_env env, void* data) {
  findCarrier* c = (findCarrier*) data;

  printf("Wait is %u.\n", c->wait);

  bool findStatus = NDIlib_find_wait_for_sources(c->find, c->wait);
  findStatus = NDIlib_find_wait_for_sources(c->find, c->wait);
  printf("Find status is %i.\n", findStatus);

  c->sources = NDIlib_find_get_current_sources(c->find, &c->no_sources);
  if (!findStatus) {
    c->status = GRANDIOSE_NOT_FOUND;
    c->errorMsg = "Did not find any NDI streams in the given wait time.";
  }
}

void findComplete(napi_env env, napi_status asyncStatus, void* data) {
  findCarrier* c = (findCarrier*) data;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async context creation failed to complete.";
  }
  if (c->status != GRANDIOSE_SUCCESS) NDIlib_find_destroy(c->find);
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_array(env, &result);
  REJECT_STATUS;

  for ( uint32_t i = 0 ; i < c->no_sources; i++ ) {
    napi_value name, uri, item;
    c->status = napi_create_string_utf8(env, c->sources[i].p_ndi_name, NAPI_AUTO_LENGTH, &name);
    REJECT_STATUS;
    c->status = napi_create_string_utf8(env, c->sources[i].p_url_address, NAPI_AUTO_LENGTH, &uri);
    REJECT_STATUS;
    c->status = napi_create_object(env, &item);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, item, "name", name);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, item, "urlAddress", uri);
    REJECT_STATUS;

    c->status = napi_set_element(env, result, i, item);
    REJECT_STATUS;
  }

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  NDIlib_find_destroy(c->find);
  tidyCarrier(env, c);
}

napi_value find(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_valuetype type;
  findCarrier* carrier = new findCarrier;

  // if (!NDIlib_initialize()) NAPI_THROW_ERROR("Failed to initialize NDI subsystem.\n");

  NDIlib_find_create_t find_create;
  find_create.show_local_sources = true;
  find_create.p_groups = nullptr;
  find_create.p_extra_ips = nullptr;

  size_t argc = 2;
  napi_value args[2];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  printf("I've got %i args.\n", argc);
  CHECK_STATUS;
  if (argc >= 1) {
    status = napi_typeof(env, args[0], &type);
    CHECK_STATUS;
    if (type == napi_object) {
      napi_value property;
      status = napi_get_named_property(env, args[0], "showLocalSources", &property);
      if (status == napi_ok) {
        status = napi_typeof(env, property, &type);
        CHECK_STATUS;
        if (type == napi_boolean) {
          status = napi_get_value_bool(env, property, &find_create.show_local_sources);
          CHECK_STATUS;
          printf("Find create show local sources %i\n", find_create.show_local_sources);
        }
      } // status == napi_ok for showLocalSources
      status = napi_get_named_property(env, args[0], "groups", &property);
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
      status = napi_get_named_property(env, args[0], "extraIPs", &property);
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

  printf("argc = %i.\n", argc);

  if (argc >= 2) {
    status = napi_typeof(env, args[1], &type);
    CHECK_STATUS;
    if (type == napi_number) {
      status = napi_get_value_uint32(env, args[1], &carrier->wait);
      CHECK_STATUS;
      printf("Set wait to %u.\n", carrier->wait);
    }
  }

  carrier->find = NDIlib_find_create_v2(&find_create);
  if (!carrier->find) NAPI_THROW_ERROR("Failed to create NDI find instance.\n");

  napi_value promise;
  status = napi_create_promise(env, &carrier->_deferred, &promise);
  CHECK_STATUS;

  napi_value resource_name;
  status = napi_create_string_utf8(env, "Find", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, findExecute,
    findComplete, carrier, &carrier->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, carrier->_request);
  CHECK_STATUS;

  return promise;
}

napi_value find_old(napi_env env, napi_callback_info info) {
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
