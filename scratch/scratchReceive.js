const g = require('../index.js');

async function run() {
  let l = await g.find();
  console.log('>>> FOUND >>>', l);
  let r = await g.receive({ source: l[0] });
  console.log('>>> RECEIVER >>>', r);
  for ( let x = 0 ; x < 10 ; x++ ) {
    let v = await r.video();
    console.log('>>> VIDEO >>>', v);
  }
  l = null;
  r = null;
  global.gc();
  setTimeout(() => { console.log("that's all folks"); }, 10000);
}

run();
