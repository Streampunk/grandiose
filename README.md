# Grandiose
[Node.js](http://nodejs.org/) native bindings to NewTek NDI(tm). For more information on NDI(tm), see:

http://NDI.NewTek.com/

This module will allow a Node.JS program to find, receive and send NDI(tm) video, audio and metadata streams over IP networks. All calls a asynchronous and use Javascript promises with all of the underlying work of NDI running on separate threads from the event loop.

NDI(tm) is a realisation of a grand vision for what IP media streams should and can be, hence a steampunk themed name of _gra-NDI-ose_.

## Installation

Grandiose supports Windows (x64), MacOS (x64 & arm64) as well as Linux (x64 only). Additional platforms may be added in the future.

Install [Node.js](http://nodejs.org/) for your platform. This software has been developed against the long term stable (LTS) release.

On Windows, the NDI(tm) DLL requires that the Visual Studio 2013 C run-times are installed, available from:

https://www.microsoft.com/en-us/download/details.aspx?id=40784

Grandiose is designed to be `require`d to use from your own application to provide async processing. For example:

    npm install --save grandiose

## Using grandiose

### Finding streams

A list of all currently available NDI(tm) sources available on the current local area network (or VLAN) can be retrieved. For example, to print a list of sources to the console, try:

```javascript
const { GrandioseFinder } = require('grandiose');

const finder = new GrandioseFinder()
setTimeout(() => {
  // Log the discovered sources after 1000ms wait
  console.log(finder.getCurrentSources())
}, 1000)
```

The result is an array, for example here are some local sources to machine :

```javascript
[ { name: 'GINGER (Intel(R) HD Graphics 520 1)',
    urlAddress: '169.254.82.1:5962' },
  { name: 'GINGER (Test Pattern)',
    urlAddress: '169.254.82.1:5961' },
  { name: 'GINGER (TOSHIBA Web Camera - HD)',
    urlAddress: '169.254.82.1:5963' } ]
```

The finder can be configured with an options object and a wait time in measured in milliseconds:

    new GrandioseFinder(<opts>);

The options are as follows:

```javascript
grandiose.find({
  // Should sources on the same system be found?
  showLocalSources: true,
  // Show only sources in a named group. May be an array.
  groups: "studio3",
  // Specific IP addresses or machine names to check
  // These are possibly on a different VLAN and not visible over MDNS
  extraIPs: [ "192.168.1.122", "mixer.studio7.zbc.com" ]
}) // ...
```

### Receiving streams

First of all, find a stream using the method above or create an object representing a source:

```javascript
const grandiose = require('grandiose');
let source = { name: "<source_name>", urlAddress: "<IP-address>:<port>" };
```

In an `async` function, create a receiver as follows:

```javascript
let receiver = await grandiose.receive({ source: source });
```

An example of the receiver object resolved by this promise is shown below:

```javascript
{ embedded: [External],
  video: [Function: video],
  audio: [Function: audio],
  metadata: [Function: metadata],
  data: [Function: data],
  source:
   { name: 'LEMARR (Test Pattern)',
     urlAddress: '169.254.82.1:5961' },
  colorFormat: 100, // grandiose.COLOR_FORMAT_FASTEST
  bandwidth: 100,   // grandiose.BANDWIDTH_HIGHEST
  allowVideoFields: true }
```

The `embedded` value is the native receiver returned by the NDI(tm) SDK. The `video`, `audio`, `metadata` and `data` functions return promises to retrieve data from the source. These promises are backed by calls that are thread safe.

The `colorFormat`, `bandwidth` and `allowVideoFields` parameters are those used to set up the receiver. These can be configured as options when creating the receiver as follows:

```javascript
let receiver = await grandiose.receive({
  source: source, // required source parameter
  // Preferred colour space - without and with alpha channel
  // One of COLOR_FORMAT_RGBX_RGBA, COLOR_FORMAT_BGRX_BGRA,
  //   COLOR_FORMAT_UYVY_RGBA, COLOR_FORMAT_UYVY_BGRA or
  //   the default of COLOR_FORMAT_FASTEST
  colorFormat: grandiose.COLOR_FORMAT_UYVY_RGBA,
  // Select bandwidth level. One of grandiose.BANDWIDTH_METADATA_ONLY,
  //   BANDWIDTH_AUDIO_ONLY, BANDWIDTH_LOWEST and the default value
  //   of BANDWIDTH_HIGHEST
  bandwidth: grandiose.BANDWIDTH_AUDIO_ONLY,
  // Set to false to receive only progressive video frames
  allowVideoFields: true, // default is true
  // An optional name for the receiver, otherwise one will be generated
  name: "rooftop"
}, );
```

#### Video

Request video frames from the source as follows:

```javascript
let timeout = 5000; // Optional timeout, default is 10000ms
try {
  for ( let x = 0 ; x < 10 ; x++) {
    let videoFrame = await receiver.video(timeout);
    console.log(videoFrame);
  }
} catch (e) { console.error(e); }
```

Here is the output associated with a video frame created by an NDI(tm) test pattern:

```javascript
{ type: 'video',
  xres: 1920,
  yres: 1080,
  frameRateN: 30000,
  frameRateD: 1001,
  pictureAspectRatio: 1.7777777910232544, // 16:9
  timestamp: [ 1538569443, 717845600 ], // PTP timestamp
  frameFormatType: 1, // grandiose.FORMAT_TYPE_INTERLACED
  timecode: [ 0, 0 ], // Measured in nanoseconds
  lineStrideBytes: 3840,
  data: <Buffer 80 10 80 10 80 10 80 10 ... > }
```

NDI presents 8-bit integer data for video.

Note that the returned promise may be rejected if the request times out or another error occurs.

The `receiver` instance will disconnect on the next garbage collection, so make sure that you don't hold onto a reference.

#### Audio

Audio follows a similar pattern to video, except that a couple of options are available to control for format of audio returned into Javasript.

```javascript
let timeout = 8000; // Optional timeout value in ms
let audioFrame = await receiver.audio({
    // One of three audio formats that NDI(tm) utilities can provide:
    //  grandiose.AUDIO_FORMAT_INT_16_INTERLEAVED,
    //  AUDIO_FORMAT_FLOAT_32_INTERLEAVED and the default value of
    //  AUDIO_FORMAT_FLOAT_32_SEPARATE
    audioFormat: grandiose.AUDIO_FORMAT_INT_16_INTERLEAVED,
    // The audio reference level in dB. This specifies how many dB above
    // the reference level (+4dBU) is the full range of integer audio.
    referenceLevel: 0 // default is 0dB
  }, timeout);
```

An example of an audio frame resolved from this promise is:

```javascript
{ type: 'audio',
  audioFormat: 2, // grandiose.AUDIO_FORMAT_INT_16_INTERLEAVED
  referenceLevel: 0, // 0dB above reference level
  sampleRate: 48000, // Hz
  channels: 4,
  samples: 4800, // Number of samples in this frame
  channelStrideInBytes: 9600, // number of bytes per channel in buffer
  timestamp: [ 1538578787, 132614500 ], // PTP timestamp
  timecode: [ 0, 800000000 ], // timecode as PTP value
  data: <Buffer 00 00 00 00 00 00 00 00 89 0a 89 0a 89 0a 89 0 ... > }
```

#### Metadata

Follows a similar pattern to video and audio, waiting for any metadata messages in the stream.

```javascript
let metadataFrame = await receiver.metadata();
```

Result is an object with a data property that is string containing the metadata, expected to be a short XML document.

#### Next available data

A means to receive the next available data payload in the stream, whether that is video, audio or metadata, allowing the application to filter the streams as required based on the `type` parameter. The optional arguments used for audio can also be used here.

```javascript
let dataFrame = await receiver.data();
if (dataFrame.type == 'video') { /* Process just the video */ }
else if (dataFrame.type == 'metadata') { console.log(dataFrame.data); }
```

### Sending streams

To follow.

### Other

To find out the version of NDI(tm), use:

    grandiose.version(); // e.g. 'NDI SDK WIN64 00:29:47 Jun 26 2018 3.5.9.0'

To check if the installed CPU is supported for NDI(tm), use:

    grandiose.isSupportedCPU(); // e.g. true

## Status, support and further development

Support for sending streams is in progress. Support for x86, Mac and Linux platforms is being considered.

Although the architecture of grandiose is such that it could be used at scale in production environments, development is not yet complete. In its current state, it is recommended that this software is used in development environments and for building prototypes. Future development will make this more appropriate for production use.

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

## License

Apart from the exceptions in the following section, this software is released under the Apache 2.0 license. Copyright 2018 Streampunk Media Ltd.

### License exceptions

The software uses libraries provided under a royalty-free license from NewTek, Inc..

* The `include` files are licensed separately by a NewTek under the MIT license.
* The NDI SDK library files are provided for convenience of installation and are covered by the NewTek license contained in the `lib` folder.

## Trademarks

NDI(tm) is a trademark of NewTek, Inc..
