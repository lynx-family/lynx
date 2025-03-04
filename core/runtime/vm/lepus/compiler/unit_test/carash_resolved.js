// Triple crash resolution

let prop = { is_format: true };
console.log(prop.is_format ? 1 : 0 == 1);

//Support Scientific notation

let num = 1.3e3 + 1.2e-3;
Assert(num == 1300.0012);

num = 1200e-1 - 13e3;
Assert(num == -12880);
