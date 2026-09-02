# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from future.utils import PY3

if PY3:
    from http.client import *
else:
    from httplib import *
    from httplib import HTTPMessage
    __future_module__ = True
