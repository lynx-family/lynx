let string = 'hello world';
Assert(typeof string == 'string');

let num = 1.3e5 + 132;
Assert(typeof num == 'number');

let array = [1, 4, 9];
Assert(typeof array == 'object');

let result = array.map(Math.sqrt);

Assert(result == [1, 2, 3]);
Assert(typeof result == 'object');

function f(x) {
  return 2 * x;
}
Assert(typeof f == 'function');

let t = f(5);
Assert(typeof t == 'number');
