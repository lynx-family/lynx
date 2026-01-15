let string = 'Hello world very nb';
let result = string.split(' ');

Assert(result == ['Hello', 'world', 'very', 'nb']);

result = string.split(' ', 3);
Assert(result == ['Hello', 'world', 'very']);

result = string.split('world', 3);
Assert(result == ['Hello ', ' very nb']);

string = '';
result = string.split('');
Assert(result == []);

string = '';
result = string.split('ds');
Assert(result == ['']);

string = 'string';
result = string.split('');
Assert(result == ['s', 't', 'r', 'i', 'n', 'g']);

string = '你 我 他 她 你我他她 他她 你我';
result = string.split(' ');
Assert(result == ['你', '我', '他', '她', '你我他她', '他她', '你我']);

result = string.split('');
Assert(
  result == ['你', ' ', '我', ' ', '他', ' ', '她', ' ', '你', '我', '他', '她', ' ', '他', '她', ' ', '你', '我']
);

result = string.split('你我');

Assert(result == ['你 我 他 她 ', '他她 他她 ', '']);
