function haveElement(x, array) {
  let length = array.length;
  for (let i = 0; i < length; i++) {
    if (x == array[i]) return true;
  }
  return false;
}

function isEqual(array1, array2) {
  if (array1.length != array2.length) {
    return false;
  }
  let length = array1.length;
  for (let i = 0; i < length; i++) {
    if (!haveElement(array1[i], array2)) {
      return false;
    }
  }
  return true;
}

let anObj = { num: 'a', key: 'b', value: 'c' };
let res = ['num', 'key', 'value'];
let result = Object.keys(anObj);

Assert(isEqual(res, result));

let arr = ['a', 'b', 'c'];

res = Object.keys(arr);
result = ['0', '1', '2'];
Assert(isEqual(res, result)); // console: ['0', '1', '2']

let a = 7.01;
let b = {
  a: 10,
  10.0: 'testNumberkey',
  20.1: 'test2',
  100: "great"
};
b[a] = 8;
let c = b[7.01];
let d = b.a;
let e = b[10.0];
let f = 20.1;
let g = b[f];
let h = b[100];
let i = 100;
let j = b[i];
b[200] = 'excellent'
let k = b[i + 100];


let aa = '7';
let bb = {};
bb[aa] = 8;
let cc = bb[aa];

Assert(c == 8);
Assert(d == 10);
Assert(e == 'testNumberkey');
Assert(g == 'test2');
Assert(h = "great")
Assert(j = 'great')
Assert( k == 'excellent')
Assert(cc == 8);
