# Copyright 2019 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import shutil
import tempfile
import unittest

from typ import artifacts
from typ.fakes.host_fake import FakeHost

class ArtifactsArtifactCreationTests(unittest.TestCase):

  def test_create_artifact_writes_to_disk_iteration_0_no_test_dir(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(output_dir, host)
    file_rel_path = host.join('stdout', 'test.jpg')
    ar.CreateArtifact('artifact_name', file_rel_path, b'contents')
    self.assertEqual(
        host.read_binary_file(
            host.join(output_dir, 'stdout', 'test.jpg')),
        b'contents')

  def test_create_artifact_writes_to_disk_iteration_1_no_test_dir(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(
      output_dir, host, iteration=1)
    file_rel_path = host.join('stdout', 'test.jpg')
    ar.CreateArtifact('artifact_name', file_rel_path, b'contents')
    self.assertEqual(
        host.read_binary_file(
            host.join(output_dir, 'retry_1', 'stdout', 'test.jpg')),
        b'contents')

  def test_create_artifact_writes_to_disk_iteration_1_test_dir(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(
      output_dir, host, iteration=1, test_name='a.b.c')
    file_rel_path = host.join('stdout', 'test.jpg')
    ar.CreateArtifact('artifact_name', file_rel_path, b'contents')
    self.assertEqual(
        host.read_binary_file(
            host.join(output_dir, 'a.b.c', 'retry_1', 'stdout', 'test.jpg')),
        b'contents')

  def test_overwriting_artifact_raises_value_error(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(
      output_dir, host, iteration=0, test_name='retry_1')
    file_rel_path = host.join('stdout', 'test.jpg')
    ar.CreateArtifact('artifact_name', file_rel_path, b'contents')
    ar1 = artifacts.Artifacts(
      output_dir, host, iteration=1)
    with self.assertRaises(ValueError) as ve:
        ar1.CreateArtifact('artifact_name', file_rel_path, b'overwritten contents')
    self.assertIn('already exists', str(ve.exception))

  def test_force_overwriting_artifact_does_not_raise_error(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(
      output_dir, host, iteration=0, test_name='a.b.c', intial_results_base_dir=True)
    file_rel_path = host.join('stdout', 'test.txt')
    ar.CreateArtifact('artifact_name', file_rel_path, 'contents',
                      write_as_text=True)
    self.assertEqual(
        host.read_text_file(
            host.join(output_dir, 'a.b.c', 'initial', 'stdout', 'test.txt')),
        'contents')
    ar.CreateArtifact('artifact_name', file_rel_path, 'overwritten contents',
                      force_overwrite=True, write_as_text=True)
    self.assertEqual(
        host.read_text_file(
            host.join(output_dir, 'a.b.c', 'initial', 'stdout', 'test.txt')),
        'overwritten contents')

  def test_create_artifact_writes_to_disk_initial_results_dir(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(
      output_dir, host, iteration=0, test_name='a.b.c', intial_results_base_dir=True)
    file_rel_path = host.join('stdout', 'test.jpg')
    ar.CreateArtifact('artifact_name', file_rel_path, b'contents')
    self.assertEqual(
        host.read_binary_file(host.join(output_dir, 'a.b.c', 'initial', 'stdout', 'test.jpg')),
        b'contents')

  def test_file_manager_writes_file(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(output_dir, host, iteration=0)
    file_path = host.join('failures', 'stderr.txt')
    ar.CreateArtifact('artifact_name', file_path, 'exception raised',
                      write_as_text=True)
    self.assertEqual(
        host.read_text_file(file_path), 'exception raised')

  def test_duplicate_artifact_raises_error_when_added_to_list(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(output_dir, host, iteration=0)
    ar.AddArtifact('artifact_name', 'foo.txt')
    with self.assertRaises(ValueError) as ve:
        ar.AddArtifact('artifact_name', 'foo.txt')
    self.assertIn('already exists', str(ve.exception))

  def test_dont_raise_value_error_for_dupl_in_add_artifacts(self):
    host = FakeHost()
    output_dir = '%stmp' % host.sep
    ar = artifacts.Artifacts(output_dir, host, iteration=0)
    ar.AddArtifact('artifact_name', 'foo.txt')
    ar.AddArtifact('artifact_name', 'foo.txt',
                    raise_exception_for_duplicates=False)
    self.assertEqual(ar.artifacts['artifact_name'], ['foo.txt'])


class ArtifactsLinkCreationTests(unittest.TestCase):
  def test_create_link(self):
    ar = artifacts.Artifacts('', FakeHost())
    ar.CreateLink('link', 'https://testsite.com')
    self.assertEqual(ar.artifacts, {'link': ['https://testsite.com']})

  def test_create_link_invalid_url(self):
    ar = artifacts.Artifacts('', FakeHost())
    with self.assertRaises(ValueError):
      ar.CreateLink('link', 'https:/malformedurl.com')

  def test_create_link_non_https(self):
    ar = artifacts.Artifacts('', FakeHost())
    with self.assertRaises(ValueError):
      ar.CreateLink('link', 'http://testsite.com')

  def test_create_link_newlines(self):
    ar = artifacts.Artifacts('', FakeHost())
    with self.assertRaises(ValueError):
      ar.CreateLink('link', 'https://some\nbadurl.com')
