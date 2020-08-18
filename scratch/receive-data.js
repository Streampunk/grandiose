/* Copyright 2018 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License")
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

const grandiose = require('../index.js')

async function run() {
  try {
    const availableSources = await grandiose.find()
    console.log('>>> FOUND >>>', availableSources)

    const receiver = await grandiose.receive({ source: availableSources[0] })
    console.log('>>> RECEIVER >>>', receiver)

    for (let x = 0; x < 1000; x++) {
      const data = await receiver.data({ audioFormat: grandiose.AUDIO_FORMAT_INT_16_INTERLEAVED, referenceLevel: 0 })
      console.log('>>> DATA >>>', data.type)
      // console.log(a.data.length)
    }
    availableSources = null
    receiver = null

    console.log(process.memoryUsage())
  } catch (e) {
    console.error(e)
  }

  if (!'gc' in global || typeof global.gc !== 'function') {
    console.log('Garbage collection is not enabled')
  } else {
    // console.log('Garbage collecting real soon:')
    setTimeout(() => {
      global.gc()
      console.log('GC 1st time. Mem usage after:', process.memoryUsage())
    }, 1000)
    setTimeout(() => {
      global.gc()
      console.log('GC 2nd time. Mem usage after:', process.memoryUsage())
    }, 2000)
    setTimeout(() => {
      global.gc()
      console.log('GC 3rd time. Mem usage after:', process.memoryUsage())
    }, 3000)
  }
}

run()
