let res = ('hello' && 'xxxx') || 'yyy';
Assert(res == 'xxxx');

res = (false && true) || true;
Assert(res == true);

res = false && (true || true);
Assert(res == false);

let a1 = true && true; // t && t return true
let a2 = true && false; // t && f return false
let a3 = false && true; // f && t return false
let a4 = false && 3 == 4; // f && f return false
let a5 = 'Cat' && 'Dog'; // t && t return "Dog"
let a6 = false && 'Cat'; // f && t return false
let a7 = 'Cat' && false; // t && f return false
let a8 = '' && false; // f && f return ""
let a9 = false && ''; // f && f return false
Assert(a1 == true);
Assert(a2 == false);
Assert(a3 == false);
Assert(a4 == false);
Assert(a5 == 'Dog');
Assert(a6 == false);
Assert(a7 == false);
Assert(a8 == '');
Assert(a9 == false);

let o1 = true || true; // t || t return true
let o2 = false || true; // f || t return true
let o3 = true || false; // t || f return true
let o4 = false || 3 == 4; // f || f return false
let o5 = 'Cat' || 'Dog'; // t || t return "Cat"
let o6 = false || 'Cat'; // f || t return "Cat"
let o7 = 'Cat' || false; // t || f return "Cat"
let o8 = '' || false; // f || f return false
let o9 = false || ''; // f || f return ""
Assert(o1 == true);
Assert(o2 == true);
Assert(o3 == true);
Assert(o4 == false);
Assert(o5 == 'Cat');
Assert(o6 == 'Cat');
Assert(o7 == 'Cat');
Assert(o8 == false);
Assert(o9 == '');

let global1 = '';
let global2 = '';
function A() {
  global1 = 'called A';
  return false;
}
function B() {
  global2 = 'called B';
  return true;
}

res = A() || B();

Assert(res == true);
Assert(global1 == 'called A');
Assert(global2 == 'called B');

global1 = '';
global2 = '';
res = A() && B();

Assert(res == false);
Assert(global1 == 'called A');
Assert(global2 == '');
