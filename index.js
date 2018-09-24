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

const addon = require('bindings')('grandiose');

const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

let find = function (args) {
  if (args == undefined) return addon.find();
  if (Array.isArray(args.groups)) {
    args.groups = args.groups.reduce((x, y) => x + ',' + y);
  }
  if (Array.isArray(args.extraIPs)) {
    args.extraIPs = args.extraIPs.reduce((x, y) => x + ',' + y);
  }
  return addon.find(args);
}

module.exports = {
  version: addon.version,
  find: find,
  isSupportedCPU: addon.isSupportedCPU
};
