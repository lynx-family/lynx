let orig = '   foo  ';

Assert(orig == '   foo  ');

Assert(orig.trim() == 'foo');

Assert(orig == '   foo  ');

// Another .trim() example, trimming from only one side

orig = 'foo    ';

Assert(orig.trim() == 'foo');
Assert(orig == 'foo    ');
