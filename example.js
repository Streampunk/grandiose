
const grandiose = require('./index')

console.log(grandiose.version())
console.log('Is CPU supported?', grandiose.isSupportedCPU())

grandiose.find()
	.then(async sources => {
		if (!sources.length) {
			return
		}
		console.log('Found sources', sources.length)

		let source = sources[0]

		console.log('Source', source)

		let receiver = await grandiose.receive({ source: source })

		let timeout = 5000; // Optional timeout, default is 10000ms
		for (let x = 0; x < 10; x++) {
			try {
				let videoFrame = await receiver.video(timeout)
				console.log(videoFrame)
			} catch (e) { console.error(e) }
		}
	})
	.catch(console.error)


