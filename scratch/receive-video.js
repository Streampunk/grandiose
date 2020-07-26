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

const g = require('../index.js')

async function run() {
  const availableSources = await g.find()
  console.log('>>> FOUND >>>', availableSources)

  const receiver = await g.receive({ source: availableSources[0] })
  console.log('>>> RECEIVER >>>', receiver)

  for (let x = 0; x < 10; x++) {
    const videoFrame = await receiver.video()

    console.log('>>> VIDEO >>>')
    console.log(videoFrame)
    console.log('-----------------')

    console.log('Mem usage:', process.memoryUsage())

    videoFrame = null
  }

  availableSources = null
  receiver = null

  console.log(process.memoryUsage())

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

run()
