let elements = ['Fire', 'Air', 'Water'];
Assert(elements.join() == 'Fire,Air,Water');
Assert(elements.join('') == 'FireAirWater');
Assert(elements.join('-') == 'Fire-Air-Water');
Assert(elements.join(' + ') == 'Fire + Air + Water');

Assert(['Fire', 'Air', 'Water'].join() == 'Fire,Air,Water')
Assert(['Fire', 'Air', 'Water'].join('') == 'FireAirWater');
Assert(['Fire', 'Air', 'Water'].join('-') == 'Fire-Air-Water');
Assert(['Fire', 'Air', 'Water'].join(' + ') == 'Fire + Air + Water');

elements = ['Fire', 2, { key: 1, value: 2 }, false];
Assert(elements.join('-') == 'Fire-2-[object Object]-false');
Assert(elements.join('-') == 'Fire-2-[object Object]-false');

elements = ['Fire', 2, [1, 2, 3], false];
console.log(elements.join() == 'Fire,2,1,2,3,false');
console.log(['Fire', 2, [1, 2, 3], false].join() == 'Fire,2,1,2,3,false');

elements = ['Fire', 2, LepusDate.now(), false];
console.log(elements.join() == 'Fire,2,2020-08-20T07:10:38.379Z,false');
