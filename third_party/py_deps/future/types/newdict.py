# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

"""
A dict subclass for Python 2 that behaves like Python 3's dict

Example use:

>>> from builtins import dict
>>> d1 = dict()    # instead of {} for an empty dict
>>> d2 = dict(key1='value1', key2='value2')

The keys, values and items methods now return iterators on Python 2.x
(with set-like behaviour on Python 2.7).

>>> for d in (d1, d2):
...     assert not isinstance(d.keys(), list)
...     assert not isinstance(d.values(), list)
...     assert not isinstance(d.items(), list)
"""

import sys

from future.utils import with_metaclass
from future.types.newobject import newobject


_builtin_dict = dict
ver = sys.version_info


class BaseNewDict(type):
    def __instancecheck__(cls, instance):
        if cls == newdict:
            return isinstance(instance, _builtin_dict)
        else:
            return issubclass(instance.__class__, cls)


class newdict(with_metaclass(BaseNewDict, _builtin_dict)):
    """
    A backport of the Python 3 dict object to Py2
    """

    if ver >= (3,):
        # Inherit items, keys and values from `dict` in 3.x
        pass
    elif ver >= (2, 7):
        items = dict.viewitems
        keys = dict.viewkeys
        values = dict.viewvalues
    else:
        items = dict.iteritems
        keys = dict.iterkeys
        values = dict.itervalues

    def __new__(cls, *args, **kwargs):
        """
        dict() -> new empty dictionary
        dict(mapping) -> new dictionary initialized from a mapping object's
            (key, value) pairs
        dict(iterable) -> new dictionary initialized as if via:
            d = {}
            for k, v in iterable:
                d[k] = v
        dict(**kwargs) -> new dictionary initialized with the name=value pairs
            in the keyword argument list.  For example:  dict(one=1, two=2)
        """

        return super(newdict, cls).__new__(cls, *args)

    def __native__(self):
        """
        Hook for the future.utils.native() function
        """
        return dict(self)


__all__ = ['newdict']
