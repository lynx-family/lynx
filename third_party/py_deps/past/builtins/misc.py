# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import unicode_literals

import inspect
import sys
import math
import numbers

from future.utils import PY2, PY3, exec_


if PY2:
    from collections import Mapping
else:
    from collections.abc import Mapping

if PY3:
    import builtins
    from collections.abc import Mapping

    def apply(f, *args, **kw):
        return f(*args, **kw)

    from past.builtins import str as oldstr

    def chr(i):
        """
        Return a byte-string of one character with ordinal i; 0 <= i <= 256
        """
        return oldstr(bytes((i,)))

    def cmp(x, y):
        """
        cmp(x, y) -> integer

        Return negative if x<y, zero if x==y, positive if x>y.
        Python2 had looser comparison allowing cmp None and non Numerical types and collections.
        Try to match the old behavior
        """
        if isinstance(x, set) and isinstance(y, set):
            raise TypeError('cannot compare sets using cmp()',)
        try:
            if isinstance(x, numbers.Number) and math.isnan(x):
                if not isinstance(y, numbers.Number):
                    raise TypeError('cannot compare float("nan"), {type_y} with cmp'.format(type_y=type(y)))
                if isinstance(y, int):
                    return 1
                else:
                    return -1
            if isinstance(y, numbers.Number) and math.isnan(y):
                if not isinstance(x, numbers.Number):
                    raise TypeError('cannot compare {type_x}, float("nan") with cmp'.format(type_x=type(x)))
                if isinstance(x, int):
                    return -1
                else:
                    return 1
            return (x > y) - (x < y)
        except TypeError:
            if x == y:
                return 0
            type_order = [
                type(None),
                numbers.Number,
                dict, list,
                set,
                (str, bytes),
            ]
            x_type_index = y_type_index = None
            for i, type_match in enumerate(type_order):
                if isinstance(x, type_match):
                    x_type_index = i
                if isinstance(y, type_match):
                    y_type_index = i
            if cmp(x_type_index, y_type_index) == 0:
                if isinstance(x, bytes) and isinstance(y, str):
                    return cmp(x.decode('ascii'), y)
                if isinstance(y, bytes) and isinstance(x, str):
                    return cmp(x, y.decode('ascii'))
                elif isinstance(x, list):
                    # if both arguments are lists take the comparison of the first non equal value
                    for x_elem, y_elem in zip(x, y):
                        elem_cmp_val = cmp(x_elem, y_elem)
                        if elem_cmp_val != 0:
                            return elem_cmp_val
                    # if all elements are equal, return equal/0
                    return 0
                elif isinstance(x, dict):
                    if len(x) != len(y):
                        return cmp(len(x), len(y))
                    else:
                        x_key = min(a for a in x if a not in y or x[a] != y[a])
                        y_key = min(b for b in y if b not in x or x[b] != y[b])
                        if x_key != y_key:
                            return cmp(x_key, y_key)
                        else:
                            return cmp(x[x_key], y[y_key])
            return cmp(x_type_index, y_type_index)

    from sys import intern

    def oct(number):
        """oct(number) -> string

        Return the octal representation of an integer
        """
        return '0' + builtins.oct(number)[2:]

    raw_input = input
    # imp was deprecated in python 3.6
    if sys.version_info >= (3, 6):
        from importlib import reload
    else:
        # for python2, python3 <= 3.4
        from imp import reload
    unicode = str
    unichr = chr
    xrange = range
else:
    import __builtin__
    from collections import Mapping
    apply = __builtin__.apply
    chr = __builtin__.chr
    cmp = __builtin__.cmp
    execfile = __builtin__.execfile
    intern = __builtin__.intern
    oct = __builtin__.oct
    raw_input = __builtin__.raw_input
    reload = __builtin__.reload
    unicode = __builtin__.unicode
    unichr = __builtin__.unichr
    xrange = __builtin__.xrange


if PY3:
    def execfile(filename, myglobals=None, mylocals=None):
        """
        Read and execute a Python script from a file in the given namespaces.
        The globals and locals are dictionaries, defaulting to the current
        globals and locals. If only globals is given, locals defaults to it.
        """
        if myglobals is None:
            # There seems to be no alternative to frame hacking here.
            caller_frame = inspect.stack()[1]
            myglobals = caller_frame[0].f_globals
            mylocals = caller_frame[0].f_locals
        elif mylocals is None:
            # Only if myglobals is given do we set mylocals to it.
            mylocals = myglobals
        if not isinstance(myglobals, Mapping):
            raise TypeError('globals must be a mapping')
        if not isinstance(mylocals, Mapping):
            raise TypeError('locals must be a mapping')
        with open(filename, "rb") as fin:
            source = fin.read()
        code = compile(source, filename, "exec")
        exec_(code, myglobals, mylocals)


if PY3:
    __all__ = ['apply', 'chr', 'cmp', 'execfile', 'intern', 'raw_input',
               'reload', 'unichr', 'unicode', 'xrange']
else:
    __all__ = []
