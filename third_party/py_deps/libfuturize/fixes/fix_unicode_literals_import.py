# Copyright (c) 2013-2024 Python Charmers, Australia
#
# This file is part of the python-future project and is distributed under
# the MIT license. See future-1.0.0.dist-info/LICENSE.txt for details.

"""
Adds this import:

    from __future__ import unicode_literals

"""

from lib2to3 import fixer_base
from libfuturize.fixer_util import future_import

class FixUnicodeLiteralsImport(fixer_base.BaseFix):
    BM_compatible = True
    PATTERN = "file_input"

    run_order = 9

    def transform(self, node, results):
        future_import(u"unicode_literals", node)
