# -*- coding: utf-8 -*-
"""
past.translation
==================

The ``past.translation`` package provides an import hook for Python 3 which
transparently runs ``futurize`` fixers over Python 2 code on import to convert
print statements into functions, etc.

It is intended to assist users in migrating to Python 3.x even if some
dependencies still only support Python 2.x.

Usage
-----

Once your Py2 package is installed in the usual module search path, the import
hook is invoked as follows:

    >>> from past.translation import autotranslate
    >>> autotranslate('mypackagename')

Or:

    >>> autotranslate(['mypackage1', 'mypackage2'])

You can unregister the hook using::

    >>> from past.translation import remove_hooks
    >>> remove_hooks()

Author: Ed Schofield.
Inspired by and based on ``uprefix`` by Vinay M. Sajip.
"""

import sys
# imp was deprecated in python 3.6
if sys.version_info >= (3, 6):
    import importlib as imp
else:
    import imp
import logging
import os
import copy
from lib2to3.pgen2.parse import ParseError
from lib2to3.refactor import RefactoringTool

from libfuturize import fixes

try:
    from importlib.machinery import (
        PathFinder,
        SourceFileLoader,
    )
except ImportError:
    PathFinder = None
    SourceFileLoader = object

if sys.version_info[:2] < (3, 4):
    import imp

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

myfixes = (list(fixes.libfuturize_fix_names_stage1) +
           list(fixes.lib2to3_fix_names_stage1) +
           list(fixes.libfuturize_fix_names_stage2) +
           list(fixes.lib2to3_fix_names_stage2))


# We detect whether the code is Py2 or Py3 by applying certain lib2to3 fixers
# to it. If the diff is empty, it's Python 3 code.

py2_detect_fixers = [
# From stage 1:
    'lib2to3.fixes.fix_apply',
    # 'lib2to3.fixes.fix_dict',        # TODO: add support for utils.viewitems() etc. and move to stage2
    'lib2to3.fixes.fix_except',
    'lib2to3.fixes.fix_execfile',
    'lib2to3.fixes.fix_exitfunc',
    'lib2to3.fixes.fix_funcattrs',
    'lib2to3.fixes.fix_filter',
    'lib2to3.fixes.fix_has_key',
    'lib2to3.fixes.fix_idioms',
    'lib2to3.fixes.fix_import',    # makes any implicit relative imports explicit. (Use with ``from __future__ import absolute_import)
    'lib2to3.fixes.fix_intern',
    'lib2to3.fixes.fix_isinstance',
    'lib2to3.fixes.fix_methodattrs',
    'lib2to3.fixes.fix_ne',
    'lib2to3.fixes.fix_numliterals',    # turns 1L into 1, 0755 into 0o755
    'lib2to3.fixes.fix_paren',
    'lib2to3.fixes.fix_print',
    'lib2to3.fixes.fix_raise',   # uses incompatible with_traceback() method on exceptions
    'lib2to3.fixes.fix_renames',
    'lib2to3.fixes.fix_reduce',
    # 'lib2to3.fixes.fix_set_literal',  # this is unnecessary and breaks Py2.6 support
    'lib2to3.fixes.fix_repr',
    'lib2to3.fixes.fix_standarderror',
    'lib2to3.fixes.fix_sys_exc',
    'lib2to3.fixes.fix_throw',
    'lib2to3.fixes.fix_tuple_params',
    'lib2to3.fixes.fix_types',
    'lib2to3.fixes.fix_ws_comma',
    'lib2to3.fixes.fix_xreadlines',

# From stage 2:
    'lib2to3.fixes.fix_basestring',
    # 'lib2to3.fixes.fix_buffer',    # perhaps not safe. Test this.
    # 'lib2to3.fixes.fix_callable',  # not needed in Py3.2+
    # 'lib2to3.fixes.fix_dict',        # TODO: add support for utils.viewitems() etc.
    'lib2to3.fixes.fix_exec',
    # 'lib2to3.fixes.fix_future',    # we don't want to remove __future__ imports
    'lib2to3.fixes.fix_getcwdu',
    # 'lib2to3.fixes.fix_imports',   # called by libfuturize.fixes.fix_future_standard_library
    # 'lib2to3.fixes.fix_imports2',  # we don't handle this yet (dbm)
    # 'lib2to3.fixes.fix_input',
    # 'lib2to3.fixes.fix_itertools',
    # 'lib2to3.fixes.fix_itertools_imports',
    'lib2to3.fixes.fix_long',
    # 'lib2to3.fixes.fix_map',
    # 'lib2to3.fixes.fix_metaclass', # causes SyntaxError in Py2! Use the one from ``six`` instead
    'lib2to3.fixes.fix_next',
    'lib2to3.fixes.fix_nonzero',     # TODO: add a decorator for mapping __bool__ to __nonzero__
    # 'lib2to3.fixes.fix_operator',    # we will need support for this by e.g. extending the Py2 operator module to provide those functions in Py3
    'lib2to3.fixes.fix_raw_input',
    # 'lib2to3.fixes.fix_unicode',   # strips off the u'' prefix, which removes a potentially helpful source of information for disambiguating unicode/byte strings
    # 'lib2to3.fixes.fix_urllib',
    'lib2to3.fixes.fix_xrange',
    # 'lib2to3.fixes.fix_zip',
]


class RTs:
    """
    A namespace for the refactoring tools. This avoids creating these at
    the module level, which slows down the module import. (See issue #117).

    There are two possible grammars: with or without the print statement.
    Hence we have two possible refactoring tool implementations.
    """
    _rt = None
    _rtp = None
    _rt_py2_detect = None
    _rtp_py2_detect = None

    @staticmethod
    def setup():
        """
        Call this before using the refactoring tools to create them on demand
        if needed.
        """
        if None in [RTs._rt, RTs._rtp]:
            RTs._rt = RefactoringTool(myfixes)
            RTs._rtp = RefactoringTool(myfixes, {'print_function': True})


    @staticmethod
    def setup_detect_python2():
        """
        Call this before using the refactoring tools to create them on demand
        if needed.
        """
        if None in [RTs._rt_py2_detect, RTs._rtp_py2_detect]:
            RTs._rt_py2_detect = RefactoringTool(py2_detect_fixers)
            RTs._rtp_py2_detect = RefactoringTool(py2_detect_fixers,
                                                  {'print_function': True})


# We need to find a prefix for the standard library, as we don't want to
# process any files there (they will already be Python 3).
#
# The following method is used by Sanjay Vinip in uprefix. This fails for
# ``conda`` environments:
#     # In a non-pythonv virtualenv, sys.real_prefix points to the installed Python.
#     # In a pythonv venv, sys.base_prefix points to the installed Python.
#     # Outside a virtual environment, sys.prefix points to the installed Python.

#     if hasattr(sys, 'real_prefix'):
#         _syslibprefix = sys.real_prefix
#     else:
#         _syslibprefix = getattr(sys, 'base_prefix', sys.prefix)

# Instead, we use the portion of the path common to both the stdlib modules
# ``math`` and ``urllib``.

def splitall(path):
    """
    Split a path into all components. From Python Cookbook.
    """
    allparts = []
    while True:
        parts = os.path.split(path)
        if parts[0] == path:  # sentinel for absolute paths
            allparts.insert(0, parts[0])
            break
        elif parts[1] == path: # sentinel for relative paths
            allparts.insert(0, parts[1])
            break
        else:
            path = parts[0]
            allparts.insert(0, parts[1])
    return allparts


def common_substring(s1, s2):
    """
    Returns the longest common substring to the two strings, starting from the
    left.
    """
    chunks = []
    path1 = splitall(s1)
    path2 = splitall(s2)
    for (dir1, dir2) in zip(path1, path2):
        if dir1 != dir2:
            break
        chunks.append(dir1)
    return os.path.join(*chunks)

# _stdlibprefix = common_substring(math.__file__, urllib.__file__)


def detect_python2(source, pathname):
    """
    Returns a bool indicating whether we think the code is Py2
    """
    RTs.setup_detect_python2()
    try:
        tree = RTs._rt_py2_detect.refactor_string(source, pathname)
    except ParseError as e:
        if e.msg != 'bad input' or e.value != '=':
            raise
        tree = RTs._rtp.refactor_string(source, pathname)

    if source != str(tree)[:-1]:   # remove added newline
        # The above fixers made changes, so we conclude it's Python 2 code
        logger.debug('Detected Python 2 code: {0}'.format(pathname))
        return True
    else:
        logger.debug('Detected Python 3 code: {0}'.format(pathname))
        return False


def transform(source, pathname):
    # This implementation uses lib2to3,
    # you can override and use something else
    # if that's better for you

    # lib2to3 likes a newline at the end
    RTs.setup()
    source += '\n'
    try:
        tree = RTs._rt.refactor_string(source, pathname)
    except ParseError as e:
        if e.msg != 'bad input' or e.value != '=':
            raise
        tree = RTs._rtp.refactor_string(source, pathname)
    # could optimise a bit for only doing str(tree) if
    # getattr(tree, 'was_changed', False) returns True
    return str(tree)[:-1]  # remove added newline


class PastSourceFileLoader(SourceFileLoader):
    exclude_paths = []
    include_paths = []

    def _convert_needed(self):
        fullname = self.name
        if any(fullname.startswith(path) for path in self.exclude_paths):
            convert = False
        elif any(fullname.startswith(path) for path in self.include_paths):
            convert = True
        else:
            convert = False
        return convert

    def _exec_transformed_module(self, module):
        source = self.get_source(self.name)
        pathname = self.path
        if detect_python2(source, pathname):
            source = transform(source, pathname)
        code = compile(source, pathname, "exec")
        exec(code, module.__dict__)

    # For Python 3.3
    def load_module(self, fullname):
        logger.debug("Running load_module for %s", fullname)
        if fullname in sys.modules:
            mod = sys.modules[fullname]
        else:
            if self._convert_needed():
                logger.debug("Autoconverting %s", fullname)
                mod = imp.new_module(fullname)
                sys.modules[fullname] = mod

                # required by PEP 302
                mod.__file__ = self.path
                mod.__loader__ = self
                if self.is_package(fullname):
                    mod.__path__ = []
                    mod.__package__ = fullname
                else:
                    mod.__package__ = fullname.rpartition('.')[0]
                self._exec_transformed_module(mod)
            else:
                mod = super().load_module(fullname)
        return mod

    # For Python >=3.4
    def exec_module(self, module):
        logger.debug("Running exec_module for %s", module)
        if self._convert_needed():
            logger.debug("Autoconverting %s", self.name)
            self._exec_transformed_module(module)
        else:
            super().exec_module(module)


class Py2Fixer(object):
    """
    An import hook class that uses lib2to3 for source-to-source translation of
    Py2 code to Py3.
    """

    # See the comments on :class:future.standard_library.RenameImport.
    # We add this attribute here so remove_hooks() and install_hooks() can
    # unambiguously detect whether the import hook is installed:
    PY2FIXER = True

    def __init__(self):
        self.found = None
        self.base_exclude_paths = ['future', 'past']
        self.exclude_paths = copy.copy(self.base_exclude_paths)
        self.include_paths = []

    def include(self, paths):
        """
        Pass in a sequence of module names such as 'plotrique.plotting' that,
        if present at the leftmost side of the full package name, would
        specify the module to be transformed from Py2 to Py3.
        """
        self.include_paths += paths

    def exclude(self, paths):
        """
        Pass in a sequence of strings such as 'mymodule' that, if
        present at the leftmost side of the full package name, would cause
        the module not to undergo any source transformation.
        """
        self.exclude_paths += paths

    # For Python 3.3
    def find_module(self, fullname, path=None):
        logger.debug("Running find_module: (%s, %s)", fullname, path)
        loader = PathFinder.find_module(fullname, path)
        if not loader:
            logger.debug("Py2Fixer could not find %s", fullname)
            return None
        loader.__class__ = PastSourceFileLoader
        loader.exclude_paths = self.exclude_paths
        loader.include_paths = self.include_paths
        return loader

    # For Python >=3.4
    def find_spec(self, fullname, path=None, target=None):
        logger.debug("Running find_spec: (%s, %s, %s)", fullname, path, target)
        spec = PathFinder.find_spec(fullname, path, target)
        if not spec:
            logger.debug("Py2Fixer could not find %s", fullname)
            return None
        spec.loader.__class__ = PastSourceFileLoader
        spec.loader.exclude_paths = self.exclude_paths
        spec.loader.include_paths = self.include_paths
        return spec


_hook = Py2Fixer()


def install_hooks(include_paths=(), exclude_paths=()):
    if isinstance(include_paths, str):
        include_paths = (include_paths,)
    if isinstance(exclude_paths, str):
        exclude_paths = (exclude_paths,)
    assert len(include_paths) + len(exclude_paths) > 0, 'Pass at least one argument'
    _hook.include(include_paths)
    _hook.exclude(exclude_paths)
    # _hook.debug = debug
    enable = sys.version_info[0] >= 3   # enabled for all 3.x+
    if enable and _hook not in sys.meta_path:
        sys.meta_path.insert(0, _hook)  # insert at beginning. This could be made a parameter

    # We could return the hook when there are ways of configuring it
    #return _hook


def remove_hooks():
    if _hook in sys.meta_path:
        sys.meta_path.remove(_hook)


def detect_hooks():
    """
    Returns True if the import hooks are installed, False if not.
    """
    return _hook in sys.meta_path
    # present = any([hasattr(hook, 'PY2FIXER') for hook in sys.meta_path])
    # return present


class hooks(object):
    """
    Acts as a context manager. Use like this:

    >>> from past import translation
    >>> with translation.hooks():
    ...     import mypy2module
    >>> import requests        # py2/3 compatible anyway
    >>> # etc.
    """
    def __enter__(self):
        self.hooks_were_installed = detect_hooks()
        install_hooks()
        return self

    def __exit__(self, *args):
        if not self.hooks_were_installed:
            remove_hooks()


class suspend_hooks(object):
    """
    Acts as a context manager. Use like this:

    >>> from past import translation
    >>> translation.install_hooks()
    >>> import http.client
    >>> # ...
    >>> with translation.suspend_hooks():
    >>>     import requests     # or others that support Py2/3

    If the hooks were disabled before the context, they are not installed when
    the context is left.
    """
    def __enter__(self):
        self.hooks_were_installed = detect_hooks()
        remove_hooks()
        return self
    def __exit__(self, *args):
        if self.hooks_were_installed:
            install_hooks()


# alias
autotranslate = install_hooks
