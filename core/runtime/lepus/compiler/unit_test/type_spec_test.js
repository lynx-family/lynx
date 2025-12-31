let s = "hello";
Assert(s.length == 5);

let ch = s.charAt(0);
Assert(ch == "h");

let sub = s.substring(1, 3);
Assert(sub == "el");

let sliced = s.slice(1, 3);
Assert(sliced == "el");

let trimmed = "  abc  ".trim();
Assert(trimmed == "abc");

let replaced = "abc".replace("a", "x");
Assert(replaced == "xbc");

let parts = "a,b,c".split(",");
Assert(parts.length == 3);
