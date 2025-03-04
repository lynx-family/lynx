# -*- coding: UTF-8 -*-

from abc import ABCMeta


class ProcessPlugin(metaclass=ABCMeta):
    def __init__(self, test):
        self._test = test

    def pre_test(self):
        pass

    def post_test(self):
        pass
    
    '''
    Execute before the case execution.
    '''
    def pre_run(self, case):
        pass
    
    '''
    Execute after the case execution is completed. 
    '''
    def post_run(self, case):
        pass
    