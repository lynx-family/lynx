let anyString = 'Mozilla';

// Outputs "Moz"
Assert(anyString.substring(0, 3) == 'Moz');
Assert(anyString.substring(3, 0) == 'Moz');
Assert(anyString.substring(3, -3) == 'Moz');
Assert(anyString.substring(-2, 3) == 'Moz');

// Outputs "lla"
Assert(anyString.substring(4, 7) == 'lla');
Assert(anyString.substring(7, 4) == 'lla');

// Outputs ""
Assert(anyString.substring(4, 4) == '');

// Outputs "Mozill"
Assert(anyString.substring(0, 6) == 'Mozill');

// Outputs "Mozilla"
Assert(anyString.substring(0, 7) == 'Mozilla');
Assert(anyString.substring(0, 10) == 'Mozilla');
