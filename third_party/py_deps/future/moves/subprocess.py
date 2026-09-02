# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import
from future.utils import PY2, PY26

from subprocess import *

if PY2:
    __future_module__ = True
    from commands import getoutput, getstatusoutput

if PY26:
    from future.backports.misc import check_output
