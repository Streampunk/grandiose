"use strict";

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
var binary = require('node-pre-gyp');

var path = require('path');

var binding_path = binary.find(path.resolve(path.join(__dirname, './package.json')));

var addon = require(binding_path);

var COLOR_FORMAT_BGRX_BGRA = 0; // No alpha channel: BGRX, Alpha channel: BGRA

var COLOR_FORMAT_UYVY_BGRA = 1; // No alpha channel: UYVY, Alpha channel: BGRA

var COLOR_FORMAT_RGBX_RGBA = 2; // No alpha channel: RGBX, Alpha channel: RGBA

var COLOR_FORMAT_UYVY_RGBA = 3; // No alpha channel: UYVY, Alpha channel: RGBA
// On Windows there are some APIs that require bottom to top images in RGBA format. Specifying
// this format will return images in this format. The image data pointer will still point to the
// "top" of the image, althought he stride will be negative. You can get the "bottom" line of the image
// using : video_data.p_data + (video_data.yres - 1)*video_data.line_stride_in_bytes

var COLOR_FORMAT_BGRX_BGRA_FLIPPED = 200;
var COLOR_FORMAT_FASTEST = 100;
var BANDWIDTH_METADATA_ONLY = -10; // Receive metadata.

var BANDWIDTH_AUDIO_ONLY = 10; // Receive metadata, audio.

var BANDWIDTH_LOWEST = 0; // Receive metadata, audio, video at a lower bandwidth and resolution.

var BANDWIDTH_HIGHEST = 100; // Receive metadata, audio, video at full resolution.

var FORMAT_TYPE_PROGRESSIVE = 1;
var FORMAT_TYPE_INTERLACED = 0;
var FORMAT_TYPE_FIELD_0 = 2;
var FORMAT_TYPE_FIELD_1 = 3; // Default NDI audio format
// Channels stored one after the other in each block - 32-bit floating point values

var AUDIO_FORMAT_FLOAT_32_SEPARATE = 0; // Alternative NDI audio foramt
// Channels stored as channel-interleaved 32-bit floating point values

var AUDIO_FORMAT_FLOAT_32_INTERLEAVED = 1; // Alternative NDI audio format
// Channels stored as channel-interleaved 16-bit integer values

var AUDIO_FORMAT_INT_16_INTERLEAVED = 2; // Find method for NDI library

var find = function find() {
  for (var _len = arguments.length, args = new Array(_len), _key = 0; _key < _len; _key++) {
    args[_key] = arguments[_key];
  }

  if (args.length === 0) return addon.find(); // If groups is passed as array, 
  // then reduce to comma-concatenated string

  if (Array.isArray(args[0].groups)) {
    args[0].groups = args[0].groups.reduce(function (x, y) {
      return x + ',' + y;
    });
  } // If extra IP addresses is passed as array, 
  // then reduce to comma-concatenated string


  if (Array.isArray(args[0].extraIPs)) {
    args[0].extraIPs = args[0].extraIPs.reduce(function (x, y) {
      return x + ',' + y;
    });
  }

  return addon.find.apply(null, args);
};

console.log('Receive:', addon.receive);
module.exports = {
  // NDI library core functions
  // NDI Find
  find: find,
  // NDI Receive
  receive: addon.receive,
  // NDI Send
  send: addon.send,
  // Send audio
  sendVideo: addon.sendVideo,
  version: addon.version,
  isSupportedCPU: addon.isSupportedCPU,
  // NDI Util constants
  COLOR_FORMAT_BGRX_BGRA: COLOR_FORMAT_BGRX_BGRA,
  COLOR_FORMAT_UYVY_BGRA: COLOR_FORMAT_UYVY_BGRA,
  COLOR_FORMAT_RGBX_RGBA: COLOR_FORMAT_RGBX_RGBA,
  COLOR_FORMAT_UYVY_RGBA: COLOR_FORMAT_UYVY_RGBA,
  COLOR_FORMAT_BGRX_BGRA_FLIPPED: COLOR_FORMAT_BGRX_BGRA_FLIPPED,
  COLOR_FORMAT_FASTEST: COLOR_FORMAT_FASTEST,
  BANDWIDTH_METADATA_ONLY: BANDWIDTH_METADATA_ONLY,
  BANDWIDTH_AUDIO_ONLY: BANDWIDTH_AUDIO_ONLY,
  BANDWIDTH_LOWEST: BANDWIDTH_LOWEST,
  BANDWIDTH_HIGHEST: BANDWIDTH_HIGHEST,
  FORMAT_TYPE_PROGRESSIVE: FORMAT_TYPE_PROGRESSIVE,
  FORMAT_TYPE_INTERLACED: FORMAT_TYPE_INTERLACED,
  FORMAT_TYPE_FIELD_0: FORMAT_TYPE_FIELD_0,
  FORMAT_TYPE_FIELD_1: FORMAT_TYPE_FIELD_1,
  AUDIO_FORMAT_FLOAT_32_SEPARATE: AUDIO_FORMAT_FLOAT_32_SEPARATE,
  AUDIO_FORMAT_FLOAT_32_INTERLEAVED: AUDIO_FORMAT_FLOAT_32_INTERLEAVED,
  AUDIO_FORMAT_INT_16_INTERLEAVED: AUDIO_FORMAT_INT_16_INTERLEAVED
};