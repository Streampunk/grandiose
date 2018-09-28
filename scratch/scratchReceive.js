const g = require('../index.js');

async function run() {
  let l = await g.find();
  console.log('>>> FOUND >>>', l);
  let r = await g.receive({ source: l[0] });
  console.log('>>> RECEIVER >>>', r);
  for ( let x = 0 ; x < 10 ; x++ ) {
    let v = await r.video();
    console.log('>>> VIDEO >>>', v);
    console.log(process.memoryUsage());
    v = null;
  }
  l = null;
  r = null;
  v = null;
  console.log(process.memoryUsage());
  setTimeout(() => { global.gc();
    console.log("that's almost all folks", process.memoryUsage()); }, 1000);
  setTimeout(() => {   global.gc(); console.log("that's it", process.memoryUsage()); }, 2000);
  setTimeout(() => {   global.gc(); console.log("that's really it", process.memoryUsage()); }, 3000);
}

run();
