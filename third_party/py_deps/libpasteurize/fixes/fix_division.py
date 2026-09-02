# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

u"""
Fixer for division: from __future__ import division if needed
"""

from lib2to3 import fixer_base
from libfuturize.fixer_util import token, future_import

def match_division(node):
    u"""
    __future__.division redefines the meaning of a single slash for division,
    so we match that and only that.
    """
    slash = token.SLASH
    return node.type == slash and not node.next_sibling.type == slash and \
                                  not node.prev_sibling.type == slash

class FixDivision(fixer_base.BaseFix):
    run_order = 4    # this seems to be ignored?

    def match(self, node):
        u"""
        Since the tree needs to be fixed once and only once if and only if it
        matches, then we can start discarding matches after we make the first.
        """
        return match_division(node)

    def transform(self, node, results):
        future_import(u"division", node)
