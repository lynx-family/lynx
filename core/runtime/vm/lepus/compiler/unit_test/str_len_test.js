let str = "1";
let len = 6;
for(let i = 0; i < len; i++) {
  if(i == 0) {
	Assert(str[i] == "1");
  }else {
	Assert(str[i] == "");
  }
}

let test_str = 'b你ea我ut他if她ul😄哈哈';

Assert(test_str.length == 17);

Assert(test_str[0] == 'b');
Assert(test_str[1] == '你');
Assert(test_str[2] == 'e');
Assert(test_str[3] == 'a');
Assert(test_str[4] == '我');
Assert(test_str[5] == 'u');
Assert(test_str[6] == 't');
Assert(test_str[7] == '他');
Assert(test_str[8] == 'i');
Assert(test_str[9] == 'f');
Assert(test_str[10] == '她');
Assert(test_str[11] == 'u');
Assert(test_str[12] == 'l');
Assert(test_str[13] == '😄');

Assert(test_str[15] == '哈');

Assert(test_str[17] == '');

str = '😄😄😄😄😄😄';

for (let i = 0; i < str.length; i++) {
  Assert(str[i] == '😄');
}

str = '测试测试测试测试';

for (let i = 0; i < str.length; i += 2) {
  Assert(str[i] == '测');
  Assert(str[i + 1] == '试');
}

str = '£0.60';

Assert(str.length == 5);

Assert(str[0] == '£');
Assert(str[1] == '0');

str = '你我他她beautiful😄😂😭😅😄£😄';

Assert(str.length == 26);
Assert(str[0] == '你');

Assert(str[4] == 'b');

Assert(str[13] == '😄');

Assert(str[14] == '😄');

Assert(str[15] == '😂');

Assert(str[16] == '😂');

Assert(str[21] == '😄');

Assert(str[23] == '£');
Assert(str[24] == '😄');

Assert(str[25] == '😄');

Assert(str[26] == '');
