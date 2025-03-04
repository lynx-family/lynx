let array1 = [];
Assert(array1.length == 0);

array1.push(1, 2, 3);
let length = array1.length;
Assert(length == 3);

let total = 0;
for (let i = 1; i <= 3; i++) {
  if (i == 2) {
    continue;
  } else {
    total = total + i;
  }
}
Assert(total == 4);

let json = '{"result":true, "count":42}';
let obj_json = JSON.parse(json);

let obj = {
  result: true,
  count: 42,
};
Assert(obj_json == obj);

let object = {
  result: true,
  count: 42,
};

let json_obj = JSON.stringify(object);
let str = '{"result":true,"count":42}';
print(json_obj);
 Assert(json_obj == str);

let Data = {
  name: '123' + '456',
  func: 'default',
};
Assert(Data.name == '123456');

let variable;
Assert(variable == null);
variable = 1;
Assert(variable == 1);

let num1 = 1;
num1 %= 10;
Assert(num1 == 1);

let object2 = {
  finally: '',
  if: 2,
};
Assert(object2.finally == '');
