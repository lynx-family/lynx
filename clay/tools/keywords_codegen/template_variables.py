#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys


def camel_case_name(name):
    return ''.join(word.capitalize() for word in name.split('-'))


def main():
    if len(sys.argv) != 4:
        raise SystemExit(
            'Usage: template_variables.py '
            '<keywords.in> <template> <output>'
        )

    keywords = []
    seen = set()
    with open(sys.argv[1], encoding='utf-8') as keywords_file:
        for line_number, line in enumerate(keywords_file, 1):
            keyword = line.strip()
            if not keyword or keyword.startswith('//'):
                continue
            if keyword in seen:
                raise ValueError(
                    f'{sys.argv[1]}:{line_number}: duplicate keyword: {keyword}'
                )
            seen.add(keyword)
            keywords.append(keyword)

    if not keywords:
        raise ValueError(f'No keywords found in {sys.argv[1]}')

    id_list = ',\n'.join(
        f'  k{camel_case_name(keyword)}' for keyword in keywords
    )
    with open(sys.argv[2], encoding='utf-8') as template_file:
        template = template_file.read()
    if template.count('{{ id_list }}') != 1:
        raise ValueError(
            f'{sys.argv[2]}: expected exactly one {{{{ id_list }}}}'
        )
    output = template.replace('{{ id_list }}', id_list)

    output_dir = os.path.dirname(sys.argv[3])
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
    with open(sys.argv[3], 'w', encoding='utf-8') as output_file:
        output_file.write(output)


if __name__ == '__main__':
    main()
