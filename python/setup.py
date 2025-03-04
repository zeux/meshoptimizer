from setuptools import setup, Extension, find_packages
import os
import platform
import sys

# Read long description from README
with open(os.path.join(os.path.dirname(__file__), "README.md"), "r", encoding="utf-8") as f:
    long_description = f.read()
import re

# Read version from package or use a default
def get_version():
    try:
        with open(os.path.join(os.path.dirname(__file__), '..', 'src', 'meshoptimizer.h'), 'r') as f:
            content = f.read()
            version_match = re.search(r'#define\s+MESHOPTIMIZER_VERSION\s+(\d+)', content)
            if version_match:
                version = int(version_match.group(1))
                major = version // 10000
                minor = (version // 100) % 100
                patch = version % 100
                return f"{major}.{minor}.{patch}"
    except:
        pass
    return '0.1.0'  # Default version if unable to extract

# Get long description from README
def get_long_description():
    try:
        with open(os.path.join(os.path.dirname(__file__), 'README.md'), 'r') as f:
            return f.read()
    except:
        return 'Python wrapper for meshoptimizer library'

# Determine source files
source_files = [
    os.path.join('..', 'src', f) for f in [
        'allocator.cpp',
        'clusterizer.cpp',
        'indexcodec.cpp',
        'indexgenerator.cpp',
        'overdrawanalyzer.cpp',
        'overdrawoptimizer.cpp',
        'simplifier.cpp',
        'spatialorder.cpp',
        'stripifier.cpp',
        'vcacheanalyzer.cpp',
        'vcacheoptimizer.cpp',
        'vertexcodec.cpp',
        'vertexfilter.cpp',
        'vfetchanalyzer.cpp',
        'vfetchoptimizer.cpp',
        'quantization.cpp',
        'partition.cpp',
    ]
]

# Platform-specific compile arguments
extra_compile_args = ['-std=c++11']
if platform.system() != 'Windows':
    extra_compile_args.append('-fPIC')
if platform.system() == 'Darwin':
    extra_compile_args.extend(['-stdlib=libc++', '-mmacosx-version-min=10.9'])

# Ensure build directories exist
build_temp_dir = os.path.join('build', f'temp.{platform.system().lower()}-{platform.machine()}-{sys.version_info[0]}.{sys.version_info[1]}')
os.makedirs(build_temp_dir, exist_ok=True)

# Define the extension module
meshoptimizer_module = Extension(
    'meshoptimizer._meshoptimizer',
    sources=source_files,
    include_dirs=[os.path.join('..', 'src')],
    extra_compile_args=extra_compile_args,
    language='c++',
)

setup(
    name='meshoptimizer',
    version=get_version(),
    description='Python wrapper for meshoptimizer library',
    long_description=get_long_description(),
    long_description_content_type='text/markdown',
    url='https://github.com/zeux/meshoptimizer',
    packages=find_packages(),
    ext_modules=[meshoptimizer_module],
    install_requires=[
        'numpy>=1.19.0',
    ],
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Topic :: Multimedia :: Graphics :: 3D Modeling',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
    keywords='mesh optimization graphics 3d',
)