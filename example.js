
const grandiose = require('./index')
console.log('NDI version:', grandiose.version())

// Guard unsupported CPU
if (!grandiose.isSupportedCPU()) {
	console.error('Unsupported CPU')
	return
}

// Optional timeout for receiving data from receiver, default is 10000ms
const timeout = 5000

// Initialize find process
grandiose.find({
	// Should sources on the same system be found?
	showLocalSources: true
})
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
		for (let x = 0; x < 10; x++) {
			try {
				const videoFrame = await receiver.video(timeout)
				console.log(videoFrame)
			} catch (e) { console.error(e) }
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


