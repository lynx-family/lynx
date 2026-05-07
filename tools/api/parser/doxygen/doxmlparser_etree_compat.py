# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from xml.etree import ElementTree


class _CompatElement(ElementTree.Element):
    @property
    def prefix(self):
        return None

    @property
    def nsmap(self):
        return {}

    @property
    def sourceline(self):
        return 0

    def getparent(self):
        return getattr(self, "_parent", None)


class _CompatEtree:
    tostring = staticmethod(ElementTree.tostring)
    SubElement = staticmethod(ElementTree.SubElement)

    @staticmethod
    def XMLParser(*args, **kwargs):
        kwargs.setdefault(
            "target", ElementTree.TreeBuilder(element_factory=_CompatElement)
        )
        return ElementTree.XMLParser(*args, **kwargs)

    @staticmethod
    def parse(infile, parser=None, **kwargs):
        doc = ElementTree.parse(infile, parser=parser or _CompatEtree.XMLParser())
        _set_parent_links(doc.getroot())
        return doc

    @staticmethod
    def fromstring(instring, parser=None, **kwargs):
        element = ElementTree.fromstring(
            instring, parser=parser or _CompatEtree.XMLParser(), **kwargs
        )
        _set_parent_links(element)
        return element


def _set_parent_links(element, parent=None):
    element._parent = parent
    for child in element:
        _set_parent_links(child, element)


def patch_doxmlparser_etree(*modules):
    for module in modules:
        if getattr(module.etree_, "__name__", "") == "xml.etree.ElementTree":
            module.etree_ = _CompatEtree
