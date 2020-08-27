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

const grandiose = require('../index')

async function run() {
  const availableSources = await grandiose.find()
  console.log('>>> FOUND SOURCES >>>', availableSources)

  const receiver = await grandiose.receive({
    source: availableSources[0],
    allowVideoFields: false
  })
  console.log('>>> RECEIVER >>>', receiver)

  for (let i = 0; i < 10; i++) {
    try {
      console.log('>>> VIDEO >>>', i)

      const videoFrame = await receiver.video()

      console.log(videoFrame)
      console.log('-----------------')

    } catch (e) {
      console.error('Error receiving frame', i, ':', e)
    }

    console.log('Mem usage:', process.memoryUsage())
    console.log('-----------------')
  }

  console.log('Mem usage:', process.memoryUsage())

  // Run garbage collection if exposed
  if (global && global.gc && typeof global.gc === 'Function') {
    setTimeout(() => {
      global.gc()
      console.log('Mem usage after GC 1st time:', process.memoryUsage())
    }, 1000)
    setTimeout(() => {
      global.gc()
      console.log('Mem usage after GC 2nd time:', process.memoryUsage())
    }, 2000)
    setTimeout(() => {
      global.gc()
      console.log('Mem usage after GC 3rd time:', process.memoryUsage())
    }, 3000)
  }
}

run()
  .catch(e => {
    console.log('Error catched in global scope', e)
  })
