Assert(LepusDate.locale() == 'en');

LepusDate.locale('zh-cn');
Assert(LepusDate.locale() == 'zh-cn');

let date1 = LepusDate.now(); // Return the current date
// console.log(date1);
let timestamp = date1.unix(); //  1596183134 Get the current timestamp
// console.log(timestamp);

let date2 = LepusDate.parse("2021-03-08 00:00:00", "YYYY-MM-DD HH:mm:ss").unix(); // Generate date object
Assert(date2 == 1615132800000);
// console.log(date2);

Assert(LepusDate.parse(date2).day() == 1)

date2 = LepusDate.parse('2018-08-08T07:59:30.123+0500'); // Generate date object
// console.log(date2);
Assert(date2.format() == '2018-08-08T07:59:30');

let date3 = LepusDate.parse(timestamp); // Generate date object from timestamp
// console.log(date3);
Assert(date3 == date1)

let timestamp2 = date2.unix(); // Return the timestamp

Assert(timestamp2 == 1533697170123);

// Format
let str1 = date2.format('YYYY-MM-DD'); // 2020-07-31
let str2 = date2.format('mm-dd'); // 07-31
// console.log(str1);
// console.log(str2);
Assert(str1 == '2018-08-08');
Assert(str2 == '59-3');
// Internationalization
Assert(LepusDate.locale() == 'zh-cn');
LepusDate.locale('en'); // Set global internationalization with the lowest priority, defaulting to built-in texts.
Assert(LepusDate.locale() == 'en');

let zhDate = LepusDate.now()
  .locale('zh-cn')
  .format('YYYY-mm-dd'); // Year (four digits) - Minute - Day
let enDate = LepusDate.now()
  .locale('en')
  .format('YY-MM-dd'); // Year (two digits) - Month - Day

// console.log(zhDate);
// console.log(enDate);

Assert(date2.year() == 2018);
Assert(date2.month() == 7);
Assert(date2.date() == 8);
Assert(date2.day() == 3);
Assert(date2.hour() == 7);
Assert(date2.minute() == 59);
Assert(date2.second() == 30);
// console.log(LepusDate.now());
Assert(date1.getTimezoneOffset() == -480);
Assert(date1.getTimezoneOffset() == date2.getTimezoneOffset());
Assert(date1.getTimezoneOffset() == LepusDate.getTimezoneOffset());

{
  let fmtZ = 'YYYY-MM-DD Z';
  let strZ1 = '2024-01-01 +05:30';
  let strZ2 = '2024-01-01 05:30';
  let strZ3 = '2024-01-01 -05:30';
  let fmtZZ = 'YYYY-MM-DD ZZ';
  let strZZ1 = '2024-01-01 +0530';
  let strZZ2 = '2024-01-01 0530';
  let strZZ3 = '2024-01-01 -0530';

  Assert(LepusDate.parse(strZ1, fmtZ).format(fmtZ) == strZ1);

  Assert(LepusDate.parse(strZ2, fmtZ).format(fmtZ) == strZ1);

  Assert(LepusDate.parse(strZ3, fmtZ).format(fmtZ) == strZ3);

  Assert(LepusDate.parse(strZZ1, fmtZZ).format(fmtZZ) == strZZ1);

  Assert(LepusDate.parse(strZZ2, fmtZZ).format(fmtZZ) == strZZ1);

  Assert(LepusDate.parse(strZZ3, fmtZZ).format(fmtZZ) == strZZ3);
}