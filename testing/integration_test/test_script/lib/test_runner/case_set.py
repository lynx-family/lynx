# -*- coding: UTF-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
@author: wangjianliang
"""
import importlib
import os
from urllib.parse import urlparse

from lib.common import utils


def underline_to_big_camel(underline_format):
    word_set = underline_format.split('_')
    for i in range(len(word_set)):
        word_set[i] = word_set[i].capitalize()
    return ''.join(word_set)


def create_instance(module_name, class_name, *args, **kwargs):
    module_meta = __import__(module_name, globals(), locals(), [class_name])
    class_meta = getattr(module_meta, class_name)
    obj = class_meta(*args, **kwargs)
    return obj

def read_all_config(case_set_path):
    json_data = {}
    paths = case_set_path.split(os.sep)
    if 'case_sets' in paths:
        case_set_name = '.'.join(paths[paths.index('case_sets')+1:])
    else:
        case_set_name = paths[-1]
    file_list = os.listdir(case_set_path)
    file_list.sort()
    for case in file_list:
        if not case.endswith('.py') or case.startswith('__') or case == 'runner.py':
            continue
        case_name = case[:-3]
        case_module = importlib.import_module(f'case_sets.{case_set_name}.{case_name}')
        if not hasattr(case_module, 'config'):
            continue
        config = case_module.config

        if 'category' not in config:
            config['category'] = case_set_name
        if 'fail_retry_time' not in config:
            config['fail_retry_time'] = 0
        if 'restart_before_exec' not in config:
            config['restart_before_exec'] = False
        json_data[case_name] = config
    return json_data

class CaseSet:
    '''Create a Caseset object that contains multiple Case objects
    
    Args:
        case_set_path: The path of the case set.
        enable_scale: Whether it is allowed to set the same resolution and width and height. Default is True.
    '''

    def __init__(self, case_set_path=None, enable_scale=True):
        self._cases = []
        json_data = read_all_config(case_set_path)

        platform = os.environ.get('platform')

        test_case_name = os.environ.get('test_case_name')
        self._category = ''

        for name, config in json_data.items():
            if test_case_name and test_case_name != name:
                continue
            if 'enable' in config.keys() and not config['enable']:
                continue
            else:
                config['enable'] = True
            if 'platform' in config.keys():
                if type(config['platform']) == str and config['platform'] != platform:
                    continue
                if type(config['platform']) == list and platform not in config['platform']:
                    continue
            self._category = config['category']
            url = f"file://lynx?local://{config['path']}.js"
            if enable_scale and 'testbench' not in config['type']:
                if platform == 'android':
                    config['width'] = config['width'] if 'width' in config else 720
                    config['height'] = config['height'] if 'height' in config else 1280
                    config['density'] = config['density'] if 'density' in config else 320
                    url = utils.append_query_to_url(url, f'width={config["width"]}&height={config["height"]}&density={config["density"]}')
                elif platform == 'ios':
                    config['width'] = config['width'] if 'width' in config else 720
                    config['height'] = config['height'] if 'height' in config else 1200
                    config['scale'] = config['scale'] if 'scale' in config else 2
                    url = utils.append_query_to_url(url, f'width={config["width"]}&height={config["height"]}&scale={config["scale"]}')
            new_case = create_instance('lib.test_runner.testcase_' + config['type'],
                                       'Testcase' + underline_to_big_camel(config['type']), name, url, config)
            self._cases.append(new_case)

    def get_cases(self):
        return self._cases
