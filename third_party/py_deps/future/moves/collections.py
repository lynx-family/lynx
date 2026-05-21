# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import
import sys

from future.utils import PY2, PY26
__future_module__ = True

from collections import *

if PY2:
    from UserDict import UserDict
    from UserList import UserList
    from UserString import UserString

if PY26:
    from future.backports.misc import OrderedDict, Counter

if sys.version_info < (3, 3):
    from future.backports.misc import ChainMap, _count_elements
