/*
The length property of a String object contains the length of the string, in UTF-16 code units. 
length is a read-only data property of string instances.
*/

let myString = "bluebells";

// Attempting to assign a value to a string's .length property has no observable effect.
myString.length = 4;

Assert(myString.length == 9)

let chinese = '中文字符'

Assert(chinese.length == 4)

let emoji = '😄😋😅😁'

Assert(emoji.length = 4)
Assert(emoji.length == 8)

let emoji_chinese = '😄😋😅😁你我他她beautiful'

Assert(emoji_chinese.length == 21)


