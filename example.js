
const grandiose = require('./index')
console.log('NDI version:', grandiose.version())

// Guard unsupported CPU
if (!grandiose.isSupportedCPU()) {
	console.error('Unsupported CPU')
	return
}

// Optional timeout for receiving data from receiver, default is 10000ms
const discoveryTimeout = 10000
const receiveTimeout = 5000

// Discover sources
grandiose.find({
	// Should sources on the same system be found?
	showLocalSources: true
}, discoveryTimeout)
	.then(async sources => {
		// Guard no sources found
		if (!sources.length) {
			console.log('No NDI sources found')
			return
		}

		console.log('Found NDI sources', sources.length)

		console.log(sources)

		const source = sources[0]

		console.log('NDI Source chosen', source)

		const receiver = await grandiose.receive({ source: source })

		// Read 10 frames
		const frameCount = {}
		for (let i = 0; i < 50; i++) {
			try {
				console.log('Requesting frame', i)
				const frame = await receiver.data(receiveTimeout) // Any type of frame
				if (!typeof frame === 'object' || !frame.type) {
					continue
				}
				// Switch based on type of frame
				switch (frame.type) {
					case 'statusChange':
						console.log('Status change:', frame)
						break
					case 'audio':
						console.log('Audio frame received', i)
						// console.log('Audio frame data:', frame)
						break
					case 'video':
						console.log('Video frame received', i)
						console.log('Video frame framerate:', frame.frameRateN / frame.frameRateD)
						break
					default:
						// Do not do anything with this type of data frame
						console.log('Unhandled type:', frame.type)
						break
				}
			} catch (e) {
				console.error('Error on requested video frame', i)
				console.error(e)
			}
		}
	})
	.catch(e => {
		if (e.code && e.code.trim() === '4040') {
			console.error('No NDI sources found...')
			return
		}

		console.error('UNKNOWN ERROR...')
		console.error(e)
	})


