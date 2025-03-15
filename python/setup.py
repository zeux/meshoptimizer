from setuptools import setup, Extension, find_packages
import os
import platform
import sys

# Get the directory containing this file (setup.py)
SETUP_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(SETUP_DIR, 'src')

# Create source directory if it doesn't exist
if not os.path.exists(SRC_DIR):
    os.makedirs(SRC_DIR)

# Copy meshoptimizer.h header if it doesn't exist in src directory
def ensure_header_file():
    header_dest = os.path.join(SRC_DIR, 'meshoptimizer.h')
    if not os.path.exists(header_dest):
        # Try to find the header file
        header_src = os.path.join('..', 'src', 'meshoptimizer.h')
        if os.path.exists(header_src):
            # Copy from parent directory
            with open(header_src, 'r') as f:
                content = f.read()
            with open(header_dest, 'w') as f:
                f.write(content)
            print(f"Copied meshoptimizer.h from {header_src} to {header_dest}")
        else:
            # Check if it's in the current directory
            header_src = os.path.join('src', 'meshoptimizer.h')
            if os.path.exists(header_src):
                with open(header_src, 'r') as f:
                    content = f.read()
                with open(header_dest, 'w') as f:
                    f.write(content)
                print(f"Copied meshoptimizer.h from {header_src} to {header_dest}")
            else:
                print("Warning: Could not find meshoptimizer.h header file")

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

# Define source files explicitly to ensure they're included in the build
def get_source_files():
    # These are the source files needed for the Python extension
    source_files = [
        'src/allocator.cpp',
        'src/clusterizer.cpp',
        'src/indexcodec.cpp',
        'src/indexgenerator.cpp',
        'src/overdrawanalyzer.cpp',
        'src/overdrawoptimizer.cpp',
        'src/partition.cpp',
        'src/quantization.cpp',
        'src/simplifier.cpp',
        'src/spatialorder.cpp',
        'src/stripifier.cpp',
        'src/vcacheanalyzer.cpp',
        'src/vcacheoptimizer.cpp',
        'src/vertexcodec.cpp',
        'src/vertexfilter.cpp',
        'src/vfetchanalyzer.cpp',
        'src/vfetchoptimizer.cpp'
    ]
    
    # Check if we're building from an sdist package
    if not os.path.exists(os.path.join('..', 'src')):
        # We're in an sdist package, source files should be in the package
        return source_files
    
    # We're building from the repository, verify files exist
    for i, src_file in enumerate(source_files):
        # Check if file exists in parent directory
        if os.path.exists(os.path.join('..', src_file)):
            continue
        # If not, check if it exists in the current directory
        elif os.path.exists(src_file):
            source_files[i] = src_file
        else:
            print(f"Warning: Source file {src_file} not found")
    
    return source_files

# Determine source files and generate module file
def generate_module_file():
    # Get source files
    source_files = get_source_files()
    # Create the module.cpp file from template
    module_template_path = os.path.join(SETUP_DIR, 'module.template.cpp')
    if not os.path.exists(module_template_path):
        return []
    # Create directory if it doesn't exist
    
    output_module_path = os.path.join(SRC_DIR, 'module.cpp')
    
    # Read template and insert source imports
    with open(module_template_path, 'r') as template_file:
        template_content = template_file.read()
    
    # Copy source files to src directory if needed
    for src_file in source_files:
        src_basename = os.path.basename(src_file)
        dest_path = os.path.join(SRC_DIR, src_basename)
        
        # If we're building from the repository, copy the files
        if os.path.exists(os.path.join('..', src_file)):
            with open(os.path.join('..', src_file), 'r') as f:
                content = f.read()
            with open(dest_path, 'w') as f:
                f.write(content)
        # If we're in an sdist package, the files might be in the current directory
        elif os.path.exists(src_file):
            with open(src_file, 'r') as f:
                content = f.read()
            with open(dest_path, 'w') as f:
                f.write(content)
    
    # Generate includes for the module.cpp file
    source_imports = '\n'.join([f'#include "{os.path.basename(src)}"' for src in source_files])
    module_content = template_content.replace('{{SOURCE_IMPORTS}}', source_imports)
    
    # Write the resulting module file
    with open(output_module_path, 'w') as module_file:
        # Add a comment indicating this file is generated
        module_file.write("// This file is automatically generated by setup.py\n")
        module_file.write(module_content)
        
    return source_files

# Platform-specific compile and link arguments
def get_build_args():
    is_windows = platform.system() == 'Windows'
    is_macos = platform.system() == 'Darwin'
    
    extra_compile_args = []
    extra_link_args = []
    define_macros = []
    
    # Define macros for all platforms
    define_macros = [
        ('MESHOPTIMIZER_IMPLEMENTATION', '1')  # Include implementation in the build
    ]
    
    if is_windows:
        # Windows-specific flags (MSVC)
        extra_compile_args = ['/std:c++14', '/O2', '/EHsc']
        # Export functions for DLL
        define_macros.extend([
            ('MESHOPTIMIZER_API', '__declspec(dllexport)'),
            ('MESHOPTIMIZER_EXPERIMENTAL', '__declspec(dllexport)')
        ])
        extra_link_args = ['/DLL']
    else:
        # Unix-like systems (Linux/Mac)
        extra_compile_args = ['-std=c++11', '-O3', '-fPIC']
        if is_macos:
            extra_compile_args.extend(['-stdlib=libc++', '-mmacosx-version-min=10.9'])
    
    return extra_compile_args, extra_link_args, define_macros

# Import numpy for include directory
import numpy as np

# Ensure header file is available
ensure_header_file()

# Generate the module file with source files
source_files = generate_module_file()

# Get the source files and build arguments
include_dirs = [SRC_DIR, np.get_include()]
# Also include parent src directory if it exists
if os.path.exists(os.path.join('..', 'src')):
    include_dirs.append(os.path.join('..', 'src'))

extra_compile_args, extra_link_args, define_macros = get_build_args()

# Define the extension module
meshoptimizer_module = Extension(
    'meshoptimizer._meshoptimizer',
    sources=["src/module.cpp"],
    include_dirs=include_dirs,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
    define_macros=define_macros,
    language='c++',
)

setup(
    name='meshoptimizer',
    version="0.2.20a5",
    description='Python wrapper for meshoptimizer library',
    long_description=get_long_description(),
    long_description_content_type='text/markdown',
    url='https://github.com/zeux/meshoptimizer',
    packages=find_packages(),
    ext_modules=[meshoptimizer_module],
    install_requires=[
        'numpy>=1.19.0',
    ],
    setup_requires=[
        'setuptools>=42',
        'wheel',
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