import pybind11
from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup
from setuptools.command.build_py import build_py as _build_py

pybind11_include_dir = pybind11.get_include()

with open("README.md") as fh:
    long_description = fh.read()


class build_py(_build_py):
    def run(self):
        _build_py.run(self)
        self.copy_file("pythonic_core.pyi", self.build_lib)


ext_modules = [
    Pybind11Extension(
        "pythonic_core",
        [
            "src/pythonic_core.cpp",
        ],
        include_dirs=[
            pybind11_include_dir,
            "/usr/include/sqlite3",
            "/usr/include/postgresql",
        ],
        libraries=["sqlite3", "pq"],
        library_dirs=["/usr/lib", "/usr/local/lib"],
        extra_compile_args=["-std=c++14"],
        extra_link_args=[
            "-lpq",
            "-lsqlite3",
        ],
    )
]

setup(
    name="pythonic_core",
    version="0.0.1",
    author="Rich Ramalho",
    author_email="richelton14@gmail.com",
    description="An extension for connecting to the database written in c/c++ to be used in Python, a performative way of making SQL queries.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/richecr/pythonic-core",
    classifiers=[
        "Intended Audience :: Information Technology",
        "Intended Audience :: System Administrators",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python",
        "Topic :: Internet",
        "Topic :: Software Development :: Libraries :: Application Frameworks",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Software Development :: Libraries",
        "Topic :: Software Development",
        "Typing :: Typed",
        "Development Status :: 4 - Beta",
        "Environment :: Web Environment",
        "Framework :: AsyncIO",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Internet :: WWW/HTTP :: HTTP Servers",
        "Topic :: Internet :: WWW/HTTP",
    ],
    ext_modules=ext_modules,
    cmdclass={"build_py": build_py},
)
