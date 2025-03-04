# -*- coding: utf8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import cv2
import time
import numpy

from lynx_e2e.api.config import settings
from lynx_e2e.api.logger import EnumLogLevel


class ImageConfig:
    def __init__(self, x_min=None, x_max=None, y_min=None, y_max=None, resize_width=None, resize_height=None,
                 out_dir="", width_scale=None, height_scale=None):
        self.x_min = x_min
        self.x_max = x_max
        self.y_min = y_min
        self.y_max = y_max
        self.resize_width = resize_width
        self.resize_height = resize_height
        self.out_dir = out_dir
        self.width_scale = width_scale
        self.height_scale = height_scale

    def update_config(self, x_min=None, x_max=None, y_min=None, y_max=None,
                      resize_width=None, resize_height=None, out_dir=None, width_scale=None, height_scale=None):
        if x_min is not None:
            self.x_min = x_min
        if x_max is not None:
            self.x_max = x_max
        if y_min is not None:
            self.y_min = y_min
        if y_max is not None:
            self.y_max = y_max
        if resize_width is not None:
            self.resize_width = resize_width
        if resize_height is not None:
            self.resize_height = resize_height
        if out_dir is not None:
            self.out_dir = out_dir
        if width_scale is not None:
            self.width_scale = width_scale
        if height_scale is not None:
            self.height_scale = height_scale

    def __str__(self):
        return f"x_min:{self.x_min}, x_max:{self.x_max}, y_min:{self.y_min}, y_max:{self.y_max}, " \
               f"resize_width:{self.resize_width}, resize_height:{self.resize_height}, out_dir:{self.out_dir}, " \
               f"width_scale:{self.width_scale}, height_scale:{self.height_scale}"


class ImageDiffMixin:

    def __init__(self, platform=os.environ.get('platform')):
        self.img_config = {}
        self.platform = platform

    def add_img_config(self, name, config: ImageConfig):
        self.img_config[name] = config

    def get_img_config(self, name) -> ImageConfig:
        if name in self.img_config.keys():
            return self.img_config[name]
        else:
            return None

    def _crop_and_resize_img(self, name):
        image = cv2.imread(name)
        if name not in self.img_config:
            return
        img_config = self.img_config[name]
        if img_config.resize_width is None or img_config.resize_height is None:
            return
        crop_img = image[int(img_config.y_min * img_config.height_scale):int(img_config.y_max * img_config.height_scale),
                         int(img_config.x_min * img_config.width_scale):int(img_config.x_max * img_config.width_scale)]
        res_img = cv2.resize(crop_img, (int(img_config.resize_width * img_config.width_scale), int(img_config.resize_height * img_config.height_scale)),
                             interpolation=cv2.INTER_CUBIC)
        ret = cv2.imwrite(os.path.join(img_config.out_dir, name), res_img)
        if ret:
            print("The screenshot has been successfully saved on %s" % os.path.join(img_config.out_dir, name))
        else:
            raise RuntimeError("Screenshot save failed: " + img_config.out_dir)

    def img_pre_process(self, name):
        '''Before comparing images, preprocess the images by resizing, converting colors, and other operations.
        Params:
            name: You can directly read an image using the following method: image = cv2.imread($name, -1).
        '''
        pass

    def diff_img(self, name, mismatch_threshold=0.01):
        self.img_pre_process(name)
        self._crop_and_resize_img(name)
        self._inner_diff_img(name, mismatch_threshold, self.current_case)

    def _inner_diff_img(self, name, mismatch_threshold, current_case):
        baseline_image_path = os.path.join(settings.PROJECT_ROOT, 'resources', self.platform, name)
        if not os.path.exists(baseline_image_path):
            baseline_image_path = os.path.join(settings.PROJECT_ROOT, 'resources', self.platform, 'testbench', name)
        if not os.path.exists(baseline_image_path):
            self.log_record("screenshot", EnumLogLevel.INFO, attachments={"test_img": name, "case_name": current_case.name}, has_error=True)
            err_message = 'No local baseline image found for comparison %s, Please prepare the images in the %s directory.' \
                          % (baseline_image_path, os.path.join('resources', self.platform))
            raise RuntimeError(err_message)
        timestamp = int(time.time())        

        test_img_path = name
        baseline_img = cv2.imread(baseline_image_path)
        baseline_img = cv2.cvtColor(baseline_img, cv2.COLOR_BGR2GRAY)
        height, width = baseline_img.shape
        test_img = cv2.imread(test_img_path)
        new_test_img_path = str(timestamp) + '--' + test_img_path.replace(".png", '_test.png')
        cv2.imwrite(new_test_img_path, test_img)
        test_img = cv2.cvtColor(test_img, cv2.COLOR_BGR2GRAY)
        test_height, test_width = test_img.shape
        if height != test_height or width != test_width:
            self.log_record("screenshot", EnumLogLevel.INFO, attachments={"test_img": new_test_img_path, "baseline_img": baseline_image_path, "case_name": current_case.name}, has_error=True)
            raise RuntimeError("User case [ {} ]: Image comparison failed, image size inconsistent, baseline_height: {}, test_height: {}, baseline_width: {}, test_width: {}".format(name, height, test_height, width, test_width))

        absdiff = cv2.absdiff(baseline_img, test_img)
        mismatch_path = str(timestamp) + '--' + test_img_path.replace(".png", '_diff.png')
        test_grey_in_rgb_channel_img = cv2.cvtColor(test_img, cv2.COLOR_GRAY2BGR)
        mask = absdiff > int(0.1 * 255)
        # (B, G, R)
        test_grey_in_rgb_channel_img[mask] = (108, 96, 244)
        cv2.imwrite(mismatch_path, test_grey_in_rgb_channel_img)

        mismatch_rate = numpy.sum(mask) / (height * width)
        self.log_record(name + " Mismatch rate:" + str(mismatch_rate * 100) + "%", EnumLogLevel.INFO, attachments={"test_img": new_test_img_path, "baseline_img": baseline_image_path, "img_diff": mismatch_path, "case_name": current_case.name}, has_error=(mismatch_rate > mismatch_threshold))
        if mismatch_rate > mismatch_threshold:
            raise RuntimeError('User case [ {} ]: Image comparison fail! Mismatch rate: {}'.format( name, str(mismatch_rate)))
