
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

		const source = sources[0]

		console.log('NDI Source chosen', source)

		const receiver = await grandiose.receive({ source: source })

		// Read 10 frames
		for (let i = 0; i < 10; i++) {
			try {
				console.log('Requesting video frame', i)
				const videoFrame = await receiver.video(receiveTimeout)
				console.log('Received video frame', i)
				console.log(videoFrame)
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


