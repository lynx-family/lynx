let str1 = 'The morning is upon us.', // The length of str1 is 23.
  str2 = str1.slice(1, 8),
  str3 = str1.slice(4, -2),
  str4 = str1.slice(12),
  str5 = str1.slice(30),
  str6 = str1.slice();
Assert(str2 == 'he morn'); // outputs: he morn
Assert(str3 == 'morning is upon u'); // outputs: morning is upon u
Assert(str4 == 'is upon us.'); // outputs: is upon us.
Assert(str5 == ''); // outputs: ""
Assert(str6 == 'The morning is upon us.');

str1 = '早晨到来了';
str2 = str1.slice(1, 2);
str3 = str1.slice(1, -2);
str4 = str1.slice(3);
str5 = str1.slice(10);
str6 = str1.slice();
Assert(str2 == '晨');
Assert(str3 == '晨到');
Assert(str4 == '来了');
Assert(str5 == '');
Assert(str6 == '早晨到来了');

str1 = "012345678";
str2 = str1.slice(-10, 3);
Assert(str2 == "012")
str2 = str1.slice(3, -10);
Assert(str2 == "");
str3 = str1.slice(3, -1);
Assert(str3 == "34567");

str4 = str1.slice(-10, -11);
Assert(str4 == "");