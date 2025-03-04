let testa = /.*/g.test();
Assert(testa == true);

let testb = /.*/g.test('你我他她');
Assert(testb == true);

let testf = /他她/.test('你我他她');
Assert(testf == true);

let testl = /你我/.test('他她');
Assert(testl == false);

// Construct a regex object using a string
let paragraph1 = 'For more information, see Chapter 1.2.4, Chapter 1.2.5';
let search1 = paragraph1.search('(Chapter \\d+(\\.\\d)*)');
Assert(search1 == 26);

let paragraph2 = 'For more information, see Chapter 1.2.4, Chapter 1.2.5';
let regex2 = /(chapter \d+(\.\d)*)/i;
let search2 = paragraph2.search(regex2);
Assert(search2 == 26);

let paragraph3 = '对于更多的信息, 参考章节1.2.4, 章节 1.2.5';
let regex3 = /(章节 \d+(\.\d)*)/i;
let search3 = paragraph3.search(regex3);
Assert(search3 == 20);

let paragraph4 = '对于更多的信息, 参考1.2.4, 1.2.5';
let regex4 = /(章节 \d+(\.\d)*)/i;
let search4 = paragraph4.search(regex4);
Assert(search4 == -1);

let paragraph5 = '对于更多的信息, 参考1.2.4, 1.2.5';
let search5 = paragraph5.search();
Assert(search5 == 0);

let matchStr1 = 'For more information, see chapt';
let matchRe1 = /(chapter \d+(\.\d)*)/i;
let matchResult1 = matchStr1.match(matchRe1);
Assert(matchResult1 == null);

let matchStr2 = '对于更多的信息, 参考章节 1.2.4, 参考章节 1.2.5';
let matchRe2 = /参考(章节 \d+(\.\d)*)/i;
let matchResult2 = matchStr2.match(matchRe2);
Assert(matchResult2.input == '对于更多的信息, 参考章节 1.2.4, 参考章节 1.2.5');
Assert(matchResult2.index == 9);
Assert(matchResult2[0] == '参考章节 1.2.4');
Assert(matchResult2[1] == '章节 1.2.4');

let matchStr3 = 'NaN means not a number. Infinity contains -Infinity and +Infinity in JavaScript.';
let matchStr4 = 'My grandfather is -65 years old and My grandmother is 63 years old.';
let matchStr5 = 'The contract was declared null and void.';
Assert(matchStr3.match('number').index == 16);
Assert(
  matchStr3.match('number').input == 'NaN means not a number. Infinity contains -Infinity and +Infinity in JavaScript.'
);
Assert(matchStr3.match('number')[0] == 'number');
Assert((matchStr5.match(null).index = 26));
Assert((matchStr5.match(null).input = 'The contract was declared null and void.'));
Assert((matchStr5.match(null)[0] = 'null'));
Assert((matchStr4.match(+65).index = 19));
Assert((matchStr4.match(+65).input = 'My grandfather is -65 years old and My grandmother is 63 years old.'));
Assert((matchStr4.match(+65)[0] = '65'));
Assert((matchStr4.match(-65).index = 18));
Assert((matchStr4.match(-65).input = 'My grandfather is -65 years old and My grandmother is 63 years old.'));
Assert((matchStr4.match(-65)[0] = '-65'));

let str1 = 'I like Spring, I love Spring, hahah';
let newstr1 = str1.replace(/spring/gi, '圣诞节');
Assert(newstr1 == 'I like 圣诞节, I love 圣诞节, hahah');

let str2 = '我喜欢春天春天, I love 春天, hahah';
let newstr2 = str2.replace(/春./gi, '圣诞节');
Assert(newstr2 == '我喜欢圣诞节圣诞节, I love 圣诞节, hahah');

let str3 = '我喜欢春天春天, I love 春天, hahah';
let newstr3 = str3.replace(/春./, '圣诞节');
Assert(newstr3 == '我喜欢圣诞节春天, I love 春天, hahah');

let reg4 = /(\w+)\s(\w+)/;
let str4 = 'John Smith';
let newstr4 = str4.replace(reg4, '$2, $1');
Assert(newstr4 == 'Smith, John');

let reg5 = /(..)\s(..)/g;
let str5 = '你我 他她你我 他她';
let newstr5 = str5.replace(reg5, '$2哈哈$1');
Assert(newstr5 == '他她哈哈你我他她哈哈你我');

let reg6 = /(他她)\s(他她)/g;
let str6 = '他她 你我';
let newstr6 = str6.replace(reg6, '$2哈哈$1');
Assert(newstr6 == '他她 你我');

let newString1 = '哈哈嘿嘿'.replace(/[哈]/g, '嘻' + '$&');
Assert(newString1 == '嘻哈嘻哈嘿嘿');

let newString2 = '哈哈嘿嘿嘻嘻'.replace(/[哈嘻]/g, '西' + '$&');
Assert(newString2 == '西哈西哈嘿嘿西嘻西嘻');

let newString3 = '哈哈嘿嘿'.replace(/[哈]/g, '嘻' + "$'");
Assert(newString3 == '嘻哈嘿嘿嘻嘿嘿嘿嘿');

let newString4 = '哈哈嘿嘿嘻嘻'.replace(/[哈嘻]/g, '西' + "$'");
Assert(newString4 == '西哈嘿嘿嘻嘻西嘿嘿嘻嘻嘿嘿西嘻西');

let newString5 = '嘿嘿'.replace(/[哈嘻]/g, '西' + '$&');
Assert(newString5 == '嘿嘿');

let newString6 = '嘿嘿'.replace(/[哈嘻]/g, '西' + "$'");
Assert(newString6 == '嘿嘿');

let newString7 = '哈嘿哈嘿123'.replace(/(哈)(嘿)/g, "$'" + '$1' + '$2');
Assert(newString7 == '哈嘿123哈嘿123哈嘿123');

function Change8(match) {
  return '-' + match;
}

let newString8 = '你我'.replace(/[你我]/g, Change8);
Assert(newString8 == '-你-我');

function Change9(match) {
  return '他她' + match;
}

let newString9 = '你我他她'.replace(/[你我]/g, Change9);
Assert(newString9 == '他她你他她我他她');

function replacer(match, p1, p2, p3, offset, string) {
  return p1 + ' ' + p2 + ' ' + p3 + ' ' + offset + ' ' + string + ' ' + ' ' + match;
}
let newString10 = 'abc12345#$*%'.replace(/([^\d]*)(\d*)([^\w]*)/, replacer);
Assert(newString10 == 'abc 12345 #$*% 0 abc12345#$*%  abc12345#$*%');

let reg = /one {({number}[^{}]+)}/;
let str = 'one {{number} mutual connnection} other {{number} mutual connections}}';
let res = str.match(reg);
Assert(res[0] == "one {{number} mutual connnection}")
Assert(res[1] == "{number} mutual connnection")
Assert(res.index == 0)