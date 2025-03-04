let arr = [1, 2, 3];
Assert(arr.includes(2) == true); // true
Assert(arr.includes(4) == false); // false
Assert(arr.includes(3, 3) == false); // false
Assert(arr.includes(3, -1) == true); // true

Assert([1, 2, 3].includes(2) == true); // true
Assert([1, 2, 3].includes(4) == false); // false
Assert([1, 2, 3].includes(3, 3) == false); // false
Assert([1, 2, 3].includes(3, -1) == true); // true

arr = ['a', 'b', 'c'];
Assert(arr.includes('c', 3) == false); // false
Assert(arr.includes('c', 100) == false); // false

Assert(['a', 'b', 'c'].includes('c', 3) == false); // false
Assert(['a', 'b', 'c'].includes('c', 100) == false); // false

arr = ['a', 'b', 'c'];
Assert(arr.includes('a', -100) == true); // true
Assert(arr.includes('b', -100) == true); // true
Assert(arr.includes('c', -100) == true); // true
Assert(arr.includes('a', -2) == false); // false

Assert(['a', 'b', 'c'].includes('a', -100) == true); // true
Assert(['a', 'b', 'c'].includes('b', -100) == true); // true
Assert(['a', 'b', 'c'].includes('c', -100) == true); // true
Assert(['a', 'b', 'c'].includes('a', -2) == false); // false
