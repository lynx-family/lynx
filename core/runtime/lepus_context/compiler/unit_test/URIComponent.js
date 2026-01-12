let set1 = ';,/?:@&=+$';
let set2 = "-_.!~*'()";
let set3 = '#';
let set4 = '你我他她';
let set5 = 'JavaScript_шеллы';
let set6 = 'ABC abc 123';
let set7 = '£0.60';

Assert(encodeURIComponent(set1) == '%3B%2C%2F%3F%3A%40%26%3D%2B%24');
Assert(encodeURIComponent(set2) == "-_.!~*'()");
Assert(encodeURIComponent(set3) == '%23');
Assert(encodeURIComponent(set4) == '%E4%BD%A0%E6%88%91%E4%BB%96%E5%A5%B9');
Assert(encodeURIComponent(set5) == 'JavaScript_%D1%88%D0%B5%D0%BB%D0%BB%D1%8B');
Assert(encodeURIComponent(set6) == 'ABC%20abc%20123');
Assert(encodeURIComponent(set7) == '%C2%A30.60');

Assert(decodeURIComponent('%3B%2C%2F%3F%3A%40%26%3D%2B%24') == set1);
Assert(decodeURIComponent("-_.!~*'()") == set2);
Assert(decodeURIComponent('%23') == set3);
Assert(decodeURIComponent('%E4%BD%A0%E6%88%91%E4%BB%96%E5%A5%B9') == set4);
Assert(decodeURIComponent('JavaScript_%D1%88%D0%B5%D0%BB%D0%BB%D1%8B') == set5);
Assert(decodeURIComponent('ABC%20abc%20123') == set6);
Assert(encodeURIComponent('%C2%A30.60') == '%25C2%25A30.60');
Assert(decodeURIComponent(set7) == '£0.60');


