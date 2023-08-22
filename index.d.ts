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
  ipAddress?: string
}

export const enum FrameType {
  Progressive = 1,
  Interlaced = 0,
  Field0 = 2,
  Field1 = 3,
}
export const FORMAT_TYPE_PROGRESSIVE: FrameType
export const FORMAT_TYPE_INTERLACED: FrameType
export const FORMAT_TYPE_FIELD_0: FrameType
export const FORMAT_TYPE_FIELD_1: FrameType

export const enum ColorFormat {
  BGRX_BGRA = 0,
  UYVY_BGRA = 1,
  RGBX_RGBA = 2,
  UYVY_RGBA = 3,
  Fastest = 100,
  Best = 101
}

export const COLOR_FORMAT_BGRX_BGRA: ColorFormat
export const COLOR_FORMAT_UYVY_BGRA: ColorFormat
export const COLOR_FORMAT_RGBX_RGBA: ColorFormat
export const COLOR_FORMAT_UYVY_RGBA: ColorFormat
export const COLOR_FORMAT_BGRX_BGRA_FLIPPED: ColorFormat
export const COLOR_FORMAT_FASTEST: ColorFormat

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

export const AUDIO_FORMAT_FLOAT_32_SEPARATE: AudioFormat
export const AUDIO_FORMAT_FLOAT_32_INTERLEAVED: AudioFormat
export const AUDIO_FORMAT_INT_16_INTERLEAVED: AudioFormat

export const enum Bandwidth {
  MetadataOnly = -10,
  AudioOnly = 10,
  Lowest = 0,
  Highest = 100
}

export const BANDWIDTH_METADATA_ONLY: Bandwidth
export const BANDWIDTH_AUDIO_ONLY: Bandwidth
export const BANDWIDTH_LOWEST: Bandwidth
export const BANDWIDTH_HIGHEST: Bandwidth

export function receive(params: {
  source: Source
  colorFormat?: ColorFormat
  bandwidth?: Bandwidth
  allowVideoFields?: boolean
  name?: string
}): Promise<Receiver>

export function send(params: {
  name: string
  groups?: string | string[]
  clockVideo?: boolean
  clockAudio?: boolean
}): Sender

/** @deprecated use GrandioseFinder instead */
export function find(params: GrandioseFinderOptions, waitMs?: number): Promise<Array<Source>>

export interface GrandioseFinderOptions {
  // Should sources on the same system be found?
  showLocalSources?: boolean,
  // Show only sources in the named groups
  groups?: string[] | string,
  // Specific IP addresses or machine names to check
  // These are possibly on a different VLAN and not visible over MDNS
  extraIPs?: string[]
}

/**
 * An instance of the NDI source finder.
 * This will monitor for sources in the background, and you can poll it for the current list at useful times.
 */
export class GrandioseFinder{
  constructor(options?: GrandioseFinderOptions)

  /** 
   * Dispose of the finder once you are finished with it
   * Failing to do so will block the application from terminating 
   */
  dispose(): void

  /**
   * Get the list of currently known Sources
   */
  getCurrentSources(): Array<Source>
}