let a = parseFloat('10');
let b = parseFloat('10.00');
let c = parseFloat('10.33');
let d = parseFloat('34 45 66');
let e = parseFloat(' 60 ');
let f = parseFloat('40 years');
let g = parseFloat('He was 40');

Assert(a == 10);
Assert(b == 10);
Assert(c == 10.33);
Assert(d == 34);
Assert(e == 60);
Assert(f == 40);
Assert(isNaN(g));

Assert(!isNaN(a));

function circumference(r) {
  return parseFloat(r) * 2.0 * 3.1415;
}

console.log(circumference(4.567));
Assert(circumference(4.567) == 28.694461);
console.log(circumference('4.567abcdefgh'));
Assert(circumference(4.567) == 28.694461);
