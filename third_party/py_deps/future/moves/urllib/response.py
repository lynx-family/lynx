# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from future import standard_library
from future.utils import PY3

if PY3:
    from urllib.response import *
else:
    __future_module__ = True
    with standard_library.suspend_hooks():
        from urllib import (addbase,
                            addclosehook,
                            addinfo,
                            addinfourl)
