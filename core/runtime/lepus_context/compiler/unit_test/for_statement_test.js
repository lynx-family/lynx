// Support for loop body as a single statement

let j = 0;
for (let i = 1; i < 10; ) Assert(i++ == ++j);
print(j);

let i = 9;
for (; j > 0; ) Assert(j-- == i--);
