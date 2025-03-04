# -*- coding: utf8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import threading

from urllib.parse import urlparse, urlunparse, parse_qsl
from lynx_e2e.api.exception import StopRunningCase
from lynx_e2e.api.logger import EnumLogLevel
from lynx_e2e.api.lynx_view import LynxView
from lynx_e2e.api.lynx_element import LynxElement
from lynx_e2e.api.upath import UPath, lynx_test_tag_


def get_lynx_view(test):
    return test.app.get_lynxview('lynxview', LynxView)

def get_lynx_element(test, lynx_test_tag) -> LynxElement:
    lynx_view = get_lynx_view(test)
    return lynx_view.get_by_test_tag(lynx_test_tag)

def take_screenshot_check(test, name_prefix, suffix, rect):
    current_case = test.current_case
    image_name = f'{current_case.image_prefix}{name_prefix}{suffix}.png'
    test.crop_image_in_lynxview(device=test.device,
                                view_rect=rect, name=image_name)
    test.diff_img(name=image_name)

def wait_for_equal(test, message, obj, prop_name, expected, timeout=10):
    """wait for a specified value and execution would stop if failed
    """
    def wait_for_equal_inner():
        lynx_test_tag = obj.get_attribute('lynx-test-tag')
        lynx_element = get_lynx_element(test, lynx_test_tag)
        actual = getattr(lynx_element, prop_name)
        if actual == expected:
            return True
        else:
            return False

    try:
        _run_with_timeout(timeout, wait_for_equal_inner)
    except TimeoutError:
        current_case = test.current_case
        image_name = f'{current_case.image_prefix}{current_case.name}_error_record.png'
        lynxview = test.app.get_lynxview('lynxview', LynxView)
        test.crop_image_in_lynxview(device=test.device,
                                view_rect=lynxview.rect, name=image_name)
        test.log_record("error_record", EnumLogLevel.INFO, attachments={"error_img": image_name}, has_error=True)
        raise StopRunningCase(message)

stop_thread = False
def _run_with_timeout(timeout, func):
    global stop_thread
    def target():
        try:
            global stop_thread
            while not stop_thread:
                result = func()
                if result:
                    break
        except Exception as e:
            stop_thread = True
            raise e

    thread = threading.Thread(target=target)
    thread.start()

    thread.join(timeout)
    if thread.is_alive():
        stop_thread = True
        raise TimeoutError("Function timed out.")

def append_query_to_url(url, query_str=None):
    def merge_query(origin_query, extra_query):
        query_list = parse_qsl(origin_query) + parse_qsl(extra_query)
        result = ''
        for item in query_list:
            if result != '':
                result = f'{result}&{item[0]}={item[1]}'
            else:
                result = f'{item[0]}={item[1]}'
        return result
    if query_str == '' or query_str is None:
        return url
    parsed_url = urlparse(url)
    if parsed_url.netloc == 'lynx':
        sub_parsed_url = urlparse(parsed_url.query)
        new_sub_url = urlunparse(sub_parsed_url._replace(query=merge_query(sub_parsed_url.query, query_str)))
        return urlunparse(parsed_url._replace(query=new_sub_url))
    else:
        return urlunparse(parsed_url._replace(query=merge_query(parsed_url.query, query_str)))
