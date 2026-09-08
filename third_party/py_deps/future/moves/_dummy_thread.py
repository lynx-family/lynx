# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import
from future.utils import PY3, PY39_PLUS


if PY39_PLUS:
    # _dummy_thread and dummy_threading modules were both deprecated in
    # Python 3.7 and removed in Python 3.9
    from _thread import *
elif PY3:
        from _dummy_thread import *
else:
    __future_module__ = True
    from dummy_thread import *
