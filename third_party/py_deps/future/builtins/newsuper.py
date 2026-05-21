# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

'''
This module provides a newsuper() function in Python 2 that mimics the
behaviour of super() in Python 3. It is designed to be used as follows:

    from __future__ import division, absolute_import, print_function
    from future.builtins import super

And then, for example:

    class VerboseList(list):
        def append(self, item):
            print('Adding an item')
            super().append(item)        # new simpler super() function

Importing this module on Python 3 has no effect.

This is based on (i.e. almost identical to) Ryan Kelly's magicsuper
module here:

    https://github.com/rfk/magicsuper.git

Excerpts from Ryan's docstring:

  "Of course, you can still explicitly pass in the arguments if you want
  to do something strange.  Sometimes you really do want that, e.g. to
  skip over some classes in the method resolution order.

  "How does it work?  By inspecting the calling frame to determine the
  function object being executed and the object on which it's being
  called, and then walking the object's __mro__ chain to find out where
  that function was defined.  Yuck, but it seems to work..."
'''

from __future__ import absolute_import
import sys
from types import FunctionType

from future.utils import PY3, PY26


_builtin_super = super

_SENTINEL = object()

def newsuper(typ=_SENTINEL, type_or_obj=_SENTINEL, framedepth=1):
    '''Like builtin super(), but capable of magic.

    This acts just like the builtin super() function, but if called
    without any arguments it attempts to infer them at runtime.
    '''
    #  Infer the correct call if used without arguments.
    if typ is _SENTINEL:
        # We'll need to do some frame hacking.
        f = sys._getframe(framedepth)

        try:
            # Get the function's first positional argument.
            type_or_obj = f.f_locals[f.f_code.co_varnames[0]]
        except (IndexError, KeyError,):
            raise RuntimeError('super() used in a function with no args')

        try:
            typ = find_owner(type_or_obj, f.f_code)
        except (AttributeError, RuntimeError, TypeError):
            # see issues #160, #267
            try:
                typ = find_owner(type_or_obj.__class__, f.f_code)
            except AttributeError:
                raise RuntimeError('super() used with an old-style class')
            except TypeError:
                raise RuntimeError('super() called outside a method')

    #  Dispatch to builtin super().
    if type_or_obj is not _SENTINEL:
        return _builtin_super(typ, type_or_obj)
    return _builtin_super(typ)


def find_owner(cls, code):
    '''Find the class that owns the currently-executing method.
    '''
    for typ in cls.__mro__:
        for meth in typ.__dict__.values():
            # Drill down through any wrappers to the underlying func.
            # This handles e.g. classmethod() and staticmethod().
            try:
                while not isinstance(meth,FunctionType):
                    if isinstance(meth, property):
                        # Calling __get__ on the property will invoke
                        # user code which might throw exceptions or have
                        # side effects
                        meth = meth.fget
                    else:
                        try:
                            meth = meth.__func__
                        except AttributeError:
                            meth = meth.__get__(cls, typ)
            except (AttributeError, TypeError):
                continue
            if meth.func_code is code:
                return typ   # Aha!  Found you.
        #  Not found! Move onto the next class in MRO.

    raise TypeError


def superm(*args, **kwds):
    f = sys._getframe(1)
    nm = f.f_code.co_name
    return getattr(newsuper(framedepth=2),nm)(*args, **kwds)


__all__ = ['newsuper']
