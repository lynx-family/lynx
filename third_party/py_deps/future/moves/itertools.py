# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import

from itertools import *
try:
    zip_longest = izip_longest
    filterfalse = ifilterfalse
except NameError:
    pass
