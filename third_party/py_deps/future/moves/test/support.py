# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

from __future__ import absolute_import

import sys

from future.standard_library import suspend_hooks
from future.utils import PY3

if PY3:
    from test.support import *
    if sys.version_info[:2] >= (3, 10):
        from test.support.os_helper import (
            EnvironmentVarGuard,
            TESTFN,
        )
        from test.support.warnings_helper import check_warnings
else:
    __future_module__ = True
    with suspend_hooks():
        from test.test_support import *
