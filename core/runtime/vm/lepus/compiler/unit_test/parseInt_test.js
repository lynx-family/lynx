let a = parseInt('10');
let b = parseInt('10.00');
let c = parseInt('10.33');
let d = parseInt('34 45 66');
let e = parseInt('   60   ');
let f = parseInt('40 years');
let g = parseInt('He was 40');
let h = parseInt('10', 10);
let i = parseInt('010');
let j = parseInt('10', 8);
let k = parseInt('0x10');
let l = parseInt('10', 16);

let m = parseInt(' 0xF', 16);

let n = parseInt('321', 2); // n is nan

let array = [[[10, 2], 100], 100];

let ret = parseInt(array, 36);

Assert(ret == 36);
Assert(isNaN(n));

Assert(m == 15);
Assert(a == 10);
Assert(b == 10);
Assert(c == 10);
Assert(d == 34);
Assert(e == 60);
Assert(f == 40);
Assert(isNaN(g));
Assert(h == 10);
Assert(i == 8);
Assert(j == 8);
Assert(k == 16);
Assert(l == 16);

Assert(parseInt('4294967295') == 4294967295);

Assert(isNaN(parseInt('')));

Assert(isNaN(parseInt('524389', 37)));