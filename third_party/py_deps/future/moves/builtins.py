# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import
from future.utils import PY3

if PY3:
    from builtins import *
else:
    __future_module__ = True
    from __builtin__ import *
    # Overwrite any old definitions with the equivalent future.builtins ones:
    from future.builtins import *
