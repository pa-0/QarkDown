#!/usr/bin/env python3

import os
import sys
import fileinput
import re


appversion_re = re.compile(r'.*appVersion\s*=\s*\{(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})\}\s*;.*')
filepath = os.path.join(sys.path[0], 'src/qarkdownapplication.cpp')

for line in fileinput.input(filepath):
    match = re.match(appversion_re, line)
    if match is None:
        continue
    print('%s.%s.%s' % (match.group(1), match.group(2), match.group(3)))
    sys.exit(0)

print('<cannot determine version!>')
sys.exit(1)
