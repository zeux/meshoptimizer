from setuptools import setup, Extension, find_packages
import os
import platform
import sys
import re

# Get the directory containing this file (setup.py)
SETUP_DIR = os.path.dirname(os.path.abspath(__file__))

# Read version from package or use a default
def get_version():
    try:
        # Try to read from the src directory first
        version_file_paths = [
            os.path.join('src', 'meshoptimizer.h'),
            os.path.join('..', 'src', 'meshoptimizer.h')
        ]
        
        for path in version_file_paths:
            full_path = os.path.join(SETUP_DIR, path)
            if os.path.exists(full_path):
                with open(full_path, 'r') as f:
                    content = f.read()
                    version_match = re.search(r'#define\s+MESHOPTIMIZER_VERSION\s+(\d+)', content)
                    if version_match:
                        version = int(version_match.group(1))
                        major = version // 10000
                        minor = (version // 100) % 100
                        patch = version % 100
                        return f"{major}.{minor}.{patch}"
    except Exception as e:
        print(f"Warning: Could not extract version: {e}")
    raise RuntimeError("Version not found. Please ensure meshoptimizer.h is present in the src directory.")

# Get long description from README
def get_long_description():
    try:
        readme_path = os.path.join(SETUP_DIR, 'README.md')
        if os.path.exists(readme_path):
            with open(readme_path, 'r', encoding='utf-8') as f:
                return f.read()
    except Exception as e:
        print(f"Warning: Could not read README.md: {e}")
    return 'Python wrapper for meshoptimizer library'

# Determine source files
# Check if we're in the python directory or the root directory
if os.path.exists(os.path.join(SETUP_DIR, 'src')):
    # Source files are in the python/src directory
    src_path = os.path.join(SETUP_DIR, 'src')
else:
    # Source files are in the root src directory
    src_path = os.path.join(SETUP_DIR, '..', 'src')

# Get all .cpp files from the src directory
source_files = []
for filename in os.listdir(src_path):
    if filename.endswith('.cpp'):
        # Use relative path for the source files
        rel_path = 'src' if os.path.exists(os.path.join(SETUP_DIR, 'src')) else os.path.join('..', 'src')
        source_files.append(os.path.join(rel_path, filename))

# Make sure we have source files
if not source_files:
    raise RuntimeError(f"No source files found in {src_path}")


# Platform-specific compile arguments
extra_compile_args = ['-std=c++11']
if platform.system() != 'Windows':
    extra_compile_args.append('-fPIC')
if platform.system() == 'Darwin':
    extra_compile_args.extend(['-stdlib=libc++', '-mmacosx-version-min=10.9'])

# Ensure build directories exist
build_temp_dir = os.path.join('build', f'temp.{platform.system().lower()}-{platform.machine()}-{sys.version_info[0]}.{sys.version_info[1]}')
os.makedirs(os.path.join(SETUP_DIR, build_temp_dir), exist_ok=True)

# Define include directories
include_dirs = []
if os.path.exists(os.path.join(SETUP_DIR, 'src')):
    include_dirs.append('src')
else:
    include_dirs.append(os.path.join('..', 'src'))

# Define the extension module
meshoptimizer_module = Extension(
    'meshoptimizer._meshoptimizer',
    sources=source_files,
    include_dirs=include_dirs,
    extra_compile_args=extra_compile_args,
    language='c++',
)

# Check if source files exist at the expected paths
def check_source_files_exist():
    for source_file in source_files:
        if not os.path.exists(source_file):
            print(f"Warning: Source file not found: {source_file}")
            return False
    return True

# Verify source files exist
if not check_source_files_exist():
    print("Warning: Some source files were not found. This may cause build failures.")
    print(f"Current directory: {os.getcwd()}")
    print(f"Setup directory: {SETUP_DIR}")

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
    package_data={
        '': ['src/*.cpp', 'src/*.h'],
    },
    include_package_data=True,
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