# Publishing to PyPI

This project includes GitHub Actions workflows to automatically build and publish the Python package to PyPI.

## Automatic Publishing

The Python package is automatically built and published to PyPI when a new GitHub release is created. This is handled by the `.github/workflows/python-publish.yml` workflow.

### Requirements

To publish to PyPI, you need to set up a PyPI API token:

1. Create an account on [PyPI](https://pypi.org/) if you don't have one
2. Go to your account settings and create an API token with upload permissions for the meshoptimizer project
3. Add the token as a GitHub repository secret named `PYPI_API_TOKEN`

### Creating a Release

To trigger the publishing workflow:

1. Go to the GitHub repository page
2. Click on "Releases" in the right sidebar
3. Click "Create a new release"
4. Enter a tag version (e.g., `v0.22.0`)
5. Enter a release title and description
6. Click "Publish release"

The workflow will automatically build the Python package and upload it to PyPI.

## Manual Building

If you want to build the package manually:

```bash
cd python
python -m pip install --upgrade pip
pip install build
python -m build
```

This will create distribution packages in the `python/dist/` directory.

## Manual Publishing

To manually publish to PyPI:

```bash
cd python
python -m pip install --upgrade pip
pip install build twine
python -m build
python -m twine upload dist/*
```

You will be prompted for your PyPI username and password.