const grandiose = require('../index.js');

// const finder = grandiose.createFinderHandle()

console.log(grandiose.version(), grandiose.isSupportedCPU())

const finder = new grandiose.GrandioseFinder({
    showLocalSources: true
})
console.log(finder)

// finder.dispose()

setInterval(() => {
    console.log(finder.getCurrentSources())
}, 500)

setTimeout(() => {
    // Dipose eventually
    finder.dispose()
}, 10000)

// grandiose.find().then(ls => {
//     console.log(ls)
// })