let str = 'abcdefghij';

Assert(String.substr(str, 1, 2) == 'bc');
Assert(String.substr(str, -3, 2) == 'hi');
Assert(String.substr(str, -3) == 'hij');
Assert(String.substr(str, 1) == 'bcdefghij');
Assert(String.substr(str, -20, 2) == 'ab');
Assert(String.substr(str, 20, 2) == '');
Assert(String.substr(str, 3, -1) == '');

let str2 = '你好hello世界w';

Assert(String.substr(str2, 1, 2) == '好h');
Assert(String.substr(str2, -3, 2) == '世界');
Assert(String.substr(str2, -3) == '世界w');
Assert(String.substr(str2, 1) == '好hello世界w');
Assert(String.substr(str2, -20, 2) == '你好');
Assert(String.substr(str2, 20, 2) == '');
Assert(String.substr(str2, 1, -1) == '');
Assert(String.substr(str2, 1, 1) == '好');
