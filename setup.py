from setuptools import setup, Extension

narytree_module = Extension(
    'narytree',
    sources=['narytreemodule_unified.cpp'],
    include_dirs=['.'],
    language='c++',
    extra_compile_args=['-std=c++17', '-O3', '-DNDEBUG'],
)

setup(
    name='narytree',
    version='1.0.0',
    author='Nico Liberato',
    author_email='nicoliberatoc@gmail.com',
    description='High-performance succinct N-ary tree with locality optimization',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://github.com/ncandio/nary_tree_improved',
    ext_modules=[narytree_module],
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Python Software Foundation License',
        'Programming Language :: Python :: 3',
        'Programming Language :: C++',
    ],
    python_requires='>=3.8',
)