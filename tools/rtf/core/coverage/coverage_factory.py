# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.coverage.jacoco_coverage import JaCoCoCoverage
from core.coverage.llvm_coverage import LLVMCoverage


def CoverageFactory(coverage_params):
    coverage_type = coverage_params["type"]
    if coverage_type == "llvm":
        return LLVMCoverage(coverage_params["ignores"], coverage_params["output"])
    elif coverage_type == "jacoco":
        return JaCoCoCoverage(
            coverage_params["output"],
            coverage_params["jacoco_cli"],
        )
    else:
        return None
