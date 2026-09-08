# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import

from future.utils import PY3

if PY3:
    from tkinter.dnd import *
else:
    try:
        from Tkdnd import *
    except ImportError:
        raise ImportError('The Tkdnd module is missing. Does your Py2 '
                          'installation include tkinter?')
