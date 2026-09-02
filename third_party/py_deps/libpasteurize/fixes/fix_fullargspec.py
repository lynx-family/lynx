# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

u"""
Fixer for getfullargspec -> getargspec
"""

from lib2to3 import fixer_base
from lib2to3.fixer_util import Name

warn_msg = u"some of the values returned by getfullargspec are not valid in Python 2 and have no equivalent."

class FixFullargspec(fixer_base.BaseFix):

    PATTERN = u"'getfullargspec'"

    def transform(self, node, results):
        self.warning(node, warn_msg)
        return Name(u"getargspec", prefix=node.prefix)
