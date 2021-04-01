export interface AudioFrame {
  type: 'audio'
  audioFormat: AudioFormat
  referenceLevel: number
  sampleRate: number // Hz
  channels: number
  samples: number
  channelStrideInBytes: number
  timestamp: [number, number] // PTP timestamp
  timecode: [number, number] // timecode as PTP value
  data: Buffer
}

export interface VideoFrame {
  type: 'video'
  xres: number
  yres: number
  frameRateN: number
  frameRateD: number
  fourCC: FourCC
  pictureAspectRatio: number
  timestamp: [ number, number ] // PTP timestamp
  frameFormatType: FrameType
  timecode: [ number, number ] // Measured in nanoseconds
  lineStrideBytes: number
  data: Buffer
}

export interface Receiver {
  embedded: unknown
  video: (timeout?: number) => Promise<VideoFrame>
  audio: (params: {
    audioFormat: AudioFormat
    referenceLevel: number
  }, timeout?: number) => Promise<AudioFrame>
  metadata: any
  data: any
  source: Source
  colorFormat: ColorFormat
  bandwidth: Bandwidth
  allowVideoFields: boolean
}

export interface Sender {
  embedded: unknown
  video: (frame: VideoFrame) => Promise<void>
  audio: (frame: AudioFrame) => Promise<void>
  name: string
  groups?: string | string[]
  clockVideo: boolean
  clockAudio: boolean
}

export interface Source {
  name: string
  urlAddress?: string
}

export const enum FrameType {
  Progressive = 1,
  Interlaced = 0,
  Field0 = 2,
  Field1 = 3,
}

export const enum ColorFormat {
  BGRX_BGRA = 0,
  UYVY_BGRA = 1,
  RGBX_RGBA = 2,
  UYVY_RGBA = 3,
  Fastest = 100,
  Best = 101
}

export const enum FourCC {
  UYVY = 1498831189,
  UYVA = 1096178005,
  P216 = 909193808,
  PA16 = 909197648,
  YV12 = 842094169,
  I420 = 808596553,
  NV12 = 842094158,
  BGRA = 1095911234,
  BGRX = 1481787202,
  RGBA = 1094862674,
  RGBX = 1480738642
}

export const enum AudioFormat {
  Float32Separate = 0,
  Float32Interleaved = 1,
  Int16Interleaved = 2
}

export const enum Bandwidth {
  MetadataOnly = -10,
  AudioOnly = 10,
  Lowest = 0,
  Highest = 100
}

export function receive(params: {
  source: Source
  colorFormat?: ColorFormat
  bandwidth?: Bandwidth
  allowVideoFields?: boolean
  name?: string
}): Receiver

export function send(params: {
  name: string
  groups?: string | string[]
  clockVideo?: boolean
  clockAudio?: boolean
}): Sender
