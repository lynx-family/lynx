let myFish = ['angel', 'clown', 'mandarin', 'surgeon'];

Assert(myFish == ['angel', 'clown', 'mandarin', 'surgeon']);
// "Before calling shift: angel, clown, mandarin, surgeon"
let shifted = myFish.shift();

Assert(myFish == ['clown', 'mandarin', 'surgeon']);

Assert(shifted == 'angel');
Assert(['angel', 'clown', 'mandarin', 'surgeon'].shift() == 'angel');