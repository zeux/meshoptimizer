from setuptools import setup, Extension, find_packages
import os
import platform

# Determine source files
source_files = [
    '../src/clusterizer.cpp',
    '../src/indexcodec.cpp',
    '../src/indexgenerator.cpp',
    '../src/overdrawanalyzer.cpp',
    '../src/overdrawoptimizer.cpp',
    '../src/simplifier.cpp',
    '../src/spatialorder.cpp',
    '../src/stripifier.cpp',
    '../src/vcacheanalyzer.cpp',
    '../src/vcacheoptimizer.cpp',
    '../src/vertexcodec.cpp',
    '../src/vertexfilter.cpp',
    '../src/vfetchanalyzer.cpp',
    '../src/vfetchoptimizer.cpp',
    '../src/allocator.cpp',
    '../src/quantization.cpp',
    '../src/partition.cpp',
]

# Define the extension module
meshoptimizer_module = Extension(
    'meshoptimizer._meshoptimizer',
    sources=source_files,
    include_dirs=['../src'],
    extra_compile_args=['-std=c++11'],
)

setup(
    name='meshoptimizer',
    version='0.1.0',
    description='Python wrapper for meshoptimizer library',
    author='Meshoptimizer Team',
    author_email='example@example.com',
    packages=find_packages(),
    ext_modules=[meshoptimizer_module],
    install_requires=[
        'numpy',
    ],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Topic :: Multimedia :: Graphics :: 3D Modeling',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
    ],
)