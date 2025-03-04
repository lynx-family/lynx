//Test String prototype
//Support String .length

let string = 'Lynx/Lepus/test.js';
let length = string.length;
Assert(length == 18);
Assert(string.length == 18);
string = '';
Assert(string.length == 0);

//Support String .trim()

string = '        hello world!     ';
Assert(string == '        hello world!     ');
Assert(string.length == 25);
Assert(string.trim() == 'hello world!');
Assert(string == '        hello world!     ');

//Support String .charAt()

let str = 'HELLO WORLD';
let res = str.charAt(); // default is 0,'H'
Assert(res == 'H');
res = str.charAt(1);
Assert(res == 'E');
res = str.charAt(12); //the result is ""
Assert(res == '');
res = str.charAt(-1); //the result is ""
Assert(res == '');

//String use [index]
str = 'HELLO WORLD';
Assert(str[0] == 'H');
Assert(str[2] == 'L');

//String compare
let str1 = 'zbs';
let str2 = 'zab';
Assert(str1 > str2);

str1 = 'Hello';
str2 = 'World';
Assert(str1 < str2);

// String supports escaping

let str3 = 'He is called "Johnny"\
\nHello Johny;\tyes';
Assert(str3 == 'He is called "Johnny"\nHello Johny;\tyes');
