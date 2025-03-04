# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


def Intersection(first, second):
    intersection = set(first) & set(second)
    return list(intersection)


def Union(first, second):
    union = set(first) | set(second)
    return list(union)


def Difference(first, second):
    difference = set(first) - set(second)
    return list(difference)
