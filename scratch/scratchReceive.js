const g = require('../index.js');

async function run() {
  let l = await g.find();
  console.log('>>> FOUND >>>', l);
  let r = await g.receive({ source: l[0] });
  console.log('>>> RECEIVER >>>', r);
  for ( let x = 0 ; x < 10 ; x++ ) {
    let v = await r.video();
    console.log('>>> VIDEO >>>', v);
    v = null;
  }
  l = null;
  r = null;
  v = null;
  setTimeout(() => { global.gc(); console.log("that's almost all folks"); }, 1000);
  setTimeout(() => {   global.gc(); console.log("that's it"); }, 2000);
  setTimeout(() => {   global.gc(); console.log("that's really it"); }, 3000);
}

run();
