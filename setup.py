#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from setuptools import setup, find_packages, command

# Mark deb package as binary
try:
    import stdeb.util
    class DebianInfo(stdeb.util.DebianInfo):
        def __init__(self, *args, **kwargs):
            kwargs['has_ext_modules'] = True
            super().__init__(*args, **kwargs)

    stdeb.util.DebianInfo = DebianInfo
except ImportError:
    pass

version = os.environ.get('VERSION')
commit = os.environ.get('COMMIT')
mode = os.environ.get('OSD_MODE')

assert version and commit

setup(
    url="http://wfb-ng.org",
    name="wfb-ng-osd-%s" % (mode,),
    version=version,
    packages=[],
    zip_safe=False,
    data_files = [('/usr/bin', ['osd.%s' % (mode,)])],
    keywords="wfb-ng-osd",
    author="Vasily Evseenko",
    author_email="svpcom@p2ptech.org",
    description="wfb-ng-osd",
    long_description='wfb-ng-osd',
    long_description_content_type='text/markdown',
    license="GPLv3",
)
