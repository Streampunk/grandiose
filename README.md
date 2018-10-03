# Grandiose
[Node.js](http://nodejs.org/) native bindings to NewTek NDI(tm). For more information on NDI(tm), see:

http://NDI.NewTek.com/

This module will allow a Node.JS program to find, receive and send NDI(tm) video, audio and metadata streams over IP networks. All calls a asynchronous and use Javascript promises with all of the underlying work of NDI running on separate threads from the event loop.

NDI(tm) is a realisation of a grand vision for what IP media streams should and can be, hence a steampunk themed name of _gra-NDI-ose_.

## Installation

Grandiose only supports Windows x64 platforms at this time. Future platforms may be added in the future.

Install [Node.js](http://nodejs.org/) for your platform. This software has been developed against the long term stable (LTS) release.

On Windows, the NDI(tm) DLL requires that the Visual Studio 2013 C run-times are installed, available from:

https://www.microsoft.com/en-us/download/details.aspx?id=40784

Grandiose is designed to be `require`d to use from your own application to provide async processing. For example:

    npm install --save grandiose

## Using grandiose

### Finding streams

A list of all currently available NDI(tm) sources available on the current local area network (or VLAN) can be retrieved. For example, to print a list of sources to the console, try:

```javascript
const grandiose = require('grandiose');

grandiose.find()
  .then(console.log)
  .catch(console.error);
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

The find operation can be configured with an options object and a wait time in measured in milliseconds:

    grandiose.find(<opts>, <wait_time>);

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
let source = { name: "<name>", urlAddress: "<IP-address>:<port>" };
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
  colorFormat: 100,
  bandwidth: 100,
  allowVideoFields: true }
```

The `embedded` value is the native receiver returned by the NDI(tm) SDK. The `video`, `audio`, `metadata` and `data` functions return promises to retrieve data from the source.

The `colorFormat`, `bandwidth` and `allowVideoFields` parameters are those used to set up the receiver. These can be configured as options when creating the receiver as follows:

```javascript
```

Request video frames from the source as follows:

```javascript
for ( let x = 0 ; x < 10 ; x++) {
  let videoFrame = await receiver.video();
  console.log(videoFrame);
}
```

Here is the output associated with a video frame created by an NDI(tm) test pattern:

```javascript

```

The receiver will close on the next garbage collection, so make sure that you don't hold onto a reference.

### Sending streams

To follow.

## Status, support and further development

Support for sending streams is in progress. Support for x86, Mac and Linux platforms is being considered.

Although the architecture of grandiose is such that it could be used at scale in production environments, development is not yet complete. In its current state, it is recommended that this software is used in development environments and for building prototypes. Future development will make this more appropriate for production use.

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

# License

This software is released under the Apache 2.0 license. Copyright 2018 Streampunk Media Ltd.

The software uses libraries provided under a royalty-free license from NewTek, Inc.. The `include` files are licensed separately by a NewTek under the MIT license. The DLL and library are provided for convenience of installation and are covered by the NewTek license contained in the `lib` folder.

# Trademarks

NDI(tm) is a trademark of NewTek, Inc..
