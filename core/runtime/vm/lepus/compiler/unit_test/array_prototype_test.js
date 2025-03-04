//test array's prototype

//Support Array pop

let array = [1, 2, 3, 'byte', 'dance', 6];
array.pop();
//print(array);   //result is [1,2,3,"byte","dance"];
Assert(array == [1, 2, 3, 'byte', 'dance']);
for (let i = 0; i < 3; i++) {
  array.pop();
}
//print(array);   //result is [1,2];
Assert(array == [1, 2]);
array.pop();
array.pop(); // now array is []
//print(array);   //result is []
Assert(array == []);
array.pop(); // array is []
// print(array);
Assert(array == []);

Assert([1, 2, 3].pop() == 2);
Assert([].pop() == 0);

//Support Array shift

let array2 = [1, 2, 3, 'byte', 'dance', 6];
//print(array2.shift());  //the result is 6
Assert(array2.shift() == 1);
//print(array2);          //the result is [1,2,3,"byte","dance"];
Assert(array2 == [2, 3, 'byte', 'dance', 6]);
array2.shift();
//print(array2);          //the result is [1,2,3,"byte"];
Assert(array2 == [3, 'byte', 'dance', 6]);
//print(array2.shift())   //the result is byte
Assert(array2.shift() == 3);
//print(array2);          //now the array2 =[1,2,3]
Assert(array2 == ['byte', 'dance', 6]);

array2.shift();
array2.shift();
array2.shift(); //now the array is []
//print(array2);
Assert(array2 == []);
//print(array2.shift())   //the result is null
Assert(!array2.shift());

//Support Array .length
let array3 = [1, 2, 3];
//console.log(array3.length);//the result is 3
Assert(array3.length == 3);
array3.push('zhang');
//console.log(array3.length);//the result is 4
Assert(array3.length == 4);
array3 = [];
//console.log(array3.length);//the result is 0
Assert(array3.length == 0);
