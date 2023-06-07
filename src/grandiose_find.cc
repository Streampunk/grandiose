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

#include <cstddef>
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

  NDIlib_find_create_t find_create;
  find_create.show_local_sources = false;
  find_create.p_groups = NULL;

  NDIlib_find_instance_t pFind = NDIlib_find_create_v2(&find_create);
  if (!pFind) {
    c->status = GRANDIOSE_ALLOCATION_FAILURE;
    c->errorMsg = "Failed to create finder.";
    return;
  }

  bool findStatus = NDIlib_find_wait_for_sources(c->find, c->wait);
  findStatus = NDIlib_find_wait_for_sources(c->find, c->wait);
  printf("Find status is %i.\n", findStatus);

  c->sources = NDIlib_find_get_current_sources(c->find, &c->no_sources);
  if (!findStatus) {
    c->status = GRANDIOSE_NOT_FOUND;
    c->errorMsg =
      "Did not find any NDI streams in the given wait time of " +
      std::to_string(c->wait) + "ms.";
  }

}

void findComplete(napi_env env, napi_status asyncStatus, void* data) {
  findCarrier* c = (findCarrier*) data;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async finder failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_array(env, &result);
  REJECT_STATUS;
  napi_value item;
  for ( uint32_t i = 0 ; i < c->no_sources; i++ ) {
    napi_value name, uri;
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

  tidyCarrier(env, c);
}

napi_value find(napi_env env, napi_callback_info info) {
  napi_valuetype type;
  findCarrier* c = new findCarrier;

  napi_value promise;
  c-> status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  NDIlib_find_create_t find_create;
  find_create.show_local_sources = true;
  find_create.p_groups = nullptr;
  find_create.p_extra_ips = nullptr;

  size_t argc = 2;
  napi_value args[2];

  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;
  if (argc >= 1) {
    c->status = napi_typeof(env, args[0], &type);
    REJECT_RETURN;
    if (type == napi_object) {
      napi_value property;
      c->status = napi_get_named_property(env, args[0], "showLocalSources", &property);
      if (c->status == napi_ok) {
        c->status = napi_typeof(env, property, &type);
        REJECT_RETURN;
        if (type == napi_boolean) {
          c->status = napi_get_value_bool(env, property, &find_create.show_local_sources);
          REJECT_RETURN;
        }
      } // status == napi_ok for showLocalSources
      c->status = napi_get_named_property(env, args[0], "groups", &property);
      if (c->status == napi_ok) {
        c->status = napi_typeof(env, property, &type);
        REJECT_RETURN;
        switch (type) {
          case napi_string:
            size_t len;
            c->status = napi_get_value_string_utf8(env, property, nullptr, 0, &len);
            REJECT_RETURN;
            find_create.p_groups = (const char *) malloc(len + 1);
            c->status = napi_get_value_string_utf8(env, property,
              (char *) find_create.p_groups, len + 1, &len);
            REJECT_RETURN;
            break;
          default:
            break;
        }
      } // status == napi_ok for p_groups
      c->status = napi_get_named_property(env, args[0], "extraIPs", &property);
      if (c->status == napi_ok) {
        c->status = napi_typeof(env, property, &type);
        REJECT_RETURN;
        switch (type) {
          case napi_string:
            size_t len;
            c->status =  napi_get_value_string_utf8(env, property, nullptr, 0, &len);
            REJECT_RETURN;
            find_create.p_extra_ips = (const char *) malloc(len + 1);
            c->status = napi_get_value_string_utf8(env, property,
              (char *) find_create.p_extra_ips, len + 1, &len);
            REJECT_RETURN;
            break;
          default:
            break;
        }
      }

    } // type == napi_object
  } // argc >= 1

  if (argc >= 2) {
    c->status = napi_typeof(env, args[1], &type);
    REJECT_RETURN;
    if (type == napi_number) {
      c->status = napi_get_value_uint32(env, args[1], &c->wait);
      REJECT_RETURN;
    }
  }

  c->find = NDIlib_find_create_v2(&find_create);
  if (!c->find) REJECT_ERROR_RETURN("Failed to create NDI find instance.", GRANDIOSE_INVALID_ARGS);

  napi_value resource_name;
  c->status = napi_create_string_utf8(env, "Find", NAPI_AUTO_LENGTH, &resource_name);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resource_name, findExecute,
    findComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

// Make a native source object from components of a source object
napi_status makeNativeSource(napi_env env, napi_value source, NDIlib_source_t *result) {
  const char* name = nullptr;
  const char* url = nullptr;
  napi_status status;
  napi_valuetype type;
  napi_value namev, urlv;
  size_t namel, urll;

  status = napi_get_named_property(env, source, "name", &namev);
  PASS_STATUS;
  status = napi_get_named_property(env, source, "urlAddress", &urlv);
  PASS_STATUS;

  status = napi_typeof(env, namev, &type);
  PASS_STATUS;
  if (type == napi_string) {
    status = napi_get_value_string_utf8(env, namev, nullptr, 0, &namel);
    PASS_STATUS;
    name = (char *) malloc(namel + 1);
    status = napi_get_value_string_utf8(env, namev, (char*) name, namel + 1, &namel);
    PASS_STATUS;
  }

  status = napi_typeof(env, urlv, &type);
  PASS_STATUS;
  if (type == napi_string) {
    status = napi_get_value_string_utf8(env, urlv, nullptr, 0, &urll);
    PASS_STATUS;
    url = (char *) malloc(urll + 1);
    status = napi_get_value_string_utf8(env, urlv, (char*) url, urll + 1, &urll);
    PASS_STATUS;
  }

  result->p_ndi_name = name;
  result->p_url_address = url;
  return napi_ok;
}

/* makeNativeSource usage example
NDIlib_source_t* fred = new NDIlib_source_t();
c->status = makeNativeSource(env, item, fred);
REJECT_STATUS;
printf("I made name=%s and urlAddress=%s\n", fred->p_ndi_name, fred->p_url_address);
delete fred;
*/
