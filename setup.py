#!/usr/bin/env python3

from distutils.core import setup, Extension
import os

# Define the extension module
narytree_module = Extension(
    'narytree',
    sources=['narytree_src/narytreemodule.cpp'],
    include_dirs=['narytree_src'],
    language='c++',
    extra_compile_args=[
        '-std=c++17',
        '-O3',
        '-march=native',
        '-DNDEBUG',
        '-ffast-math',
        '-funroll-loops',
        '-fprefetch-loop-arrays',
        '-finline-functions',
        '-Wall',
        '-Wno-unused-parameter',
        '-Wno-missing-field-initializers'
    ],
    extra_link_args=[
        '-O3',
        '-flto',
    ],
    define_macros=[
        ('SUCCINCT_NARYTREE_UNIFIED', '1'),
        ('LOCALITY_OPTIMIZED', '1'),
        ('LAZY_BALANCING_ENABLED', '1'),
    ]
)

setup(
    name='narytree',
    version='1.0.0',
    description='Complete N-ary Tree with full API: self-balancing, succinct encoding, and locality optimization',
    long_description='C++17 N-ary tree with complete documented API including: balance_tree(), needs_rebalancing(), encode_succinct(), decode_succinct(), enable_array_storage(), calculate_locality_score(), and full node-level traversal methods',
    author='Nico Liberato',
    author_email='nicoliberatoc@gmail.com',
    ext_modules=[narytree_module],
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Topic :: Scientific/Engineering :: Information Analysis',
    ],
    python_requires='>=3.8',
)
