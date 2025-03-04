# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import json
import os
import shutil
import subprocess

from core.coverage.coverage import Coverage
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log
from core.utils.xml_reader import XmlReader


class JaCoCoCoverage(Coverage):
    def __init__(self, output, cli_path):
        super().__init__()
        self.output = output
        self.cli_path = cli_path

    def __gen_xml_report(self, targets: [Target]):
        Log.info(f"Generate xml report...")
        xml_store_path = os.path.join(
            RTFEnv.get_project_root_path(), self.output, "xml"
        )
        if not os.path.exists(xml_store_path):
            os.makedirs(xml_store_path)
        for target in targets:
            if "test_xml" in target.global_info:
                shutil.copy(target.global_info["test_xml"], xml_store_path)

    def __get_ec_file_list(self, targets: [Target]):
        result = []
        for target in targets:
            if target.coverage:
                result.append(target.get_coverage_raw_data())
        return result

    def __get_class_files_list(self, targets: [Target]):
        result = []
        for target in targets:
            if target.coverage:
                result.append(f"--classfiles {target.params['class_files']}")
        return result

    def __get_source_files_list(self, targets: [Target]):
        result = []
        for target in targets:
            if target.coverage:
                result.append(f"--sourcefiles {target.params['class_files']}")
        return result

    def __gen_jacoco_xml_report(self, targets: [Target]):
        Log.info(f"Generate jacoco xml report...")
        jacoco_xml_store_path = os.path.join(
            RTFEnv.get_project_root_path(), self.output, "jacoco_xml"
        )
        if not os.path.exists(jacoco_xml_store_path):
            os.makedirs(jacoco_xml_store_path)
        xml_file_path = os.path.join(jacoco_xml_store_path, "coverage.xml")
        ec_files = self.__get_ec_file_list(targets)
        if len(ec_files) == 0:
            return None
        class_files_args = " ".join(self.__get_class_files_list(targets))
        source_files_args = " ".join(self.__get_source_files_list(targets))
        cmd = f"java -jar {self.cli_path} report {' '.join(ec_files)} --xml {xml_file_path} {class_files_args} {source_files_args}"
        subprocess.check_call(cmd, shell=True)
        return xml_file_path

    def __gen_jacoco_html_report(self, targets: [Target]):
        Log.info(f"Generate jacoco html report...")
        jacoco_html_store_path = os.path.join(
            RTFEnv.get_project_root_path(), self.output, "jacoco_html"
        )
        if not os.path.exists(jacoco_html_store_path):
            os.makedirs(jacoco_html_store_path)
        ec_files = self.__get_ec_file_list(targets)
        class_files_args = " ".join(self.__get_class_files_list(targets))
        source_files_args = " ".join(self.__get_source_files_list(targets))
        cmd = f"java -jar {self.cli_path} report {' '.join(ec_files)} --html {jacoco_html_store_path} {class_files_args} {source_files_args}"
        subprocess.check_call(cmd, shell=True)

    def __gen_coverage_summary_json(self, xml_file_path):
        Log.info(f"Generate json summary report...")
        coverage_summary_json_store_path = os.path.join(
            RTFEnv.get_project_root_path(), self.output, "json"
        )
        if not os.path.exists(coverage_summary_json_store_path):
            os.makedirs(coverage_summary_json_store_path)
        root = XmlReader(xml_file_path)
        summary_json = {}
        COUNTER_NAME_MAPPING = {
            "INSTRUCTION": "instantiations",
            "BRANCH": "branches",
            "LINE": "lines",
            "METHOD": "functions",
        }
        for child in root:
            if child.tag == "counter" and child.attrib["type"] in COUNTER_NAME_MAPPING:
                missed_int = int(child.attrib["missed"])
                covered_int = int(child.attrib["covered"])
                summary_json.update(
                    {
                        COUNTER_NAME_MAPPING[child.attrib["type"]]: {
                            "count": missed_int + covered_int,
                            "covered": covered_int,
                            "percent": covered_int * 100.0 / (missed_int + covered_int),
                        }
                    }
                )
        with open(
            os.path.join(
                coverage_summary_json_store_path, "coverage_summary_total.json"
            ),
            "w",
        ) as wf:
            json.dump(summary_json, wf)

    def gen_report(self, targets: [Target]):
        try:
            self.__gen_xml_report(targets)
            xml_file_path = self.__gen_jacoco_xml_report(targets)
            if xml_file_path is None:
                Log.info(f"No coverage data, skip...")
                return
            self.__gen_coverage_summary_json(xml_file_path)
            self.__gen_jacoco_html_report(targets)
        except Exception as e:
            Log.fatal(f"Generate coverage report failed! {e}")
