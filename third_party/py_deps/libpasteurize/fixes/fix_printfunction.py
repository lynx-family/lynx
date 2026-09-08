# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

u"""
Fixer for print: from __future__ import print_function.
"""

from lib2to3 import fixer_base
from libfuturize.fixer_util import future_import

class FixPrintfunction(fixer_base.BaseFix):

    # explicit = True

    PATTERN = u"""
              power< 'print' trailer < '(' any* ')' > any* >
              """

    def transform(self, node, results):
        future_import(u"print_function", node)
