let a = 1;
Assert(globalThis.a === 1);

a = "string";
Assert(globalThis.a === "string");
