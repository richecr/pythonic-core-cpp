import pybind11
from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup
from setuptools.command.build_py import build_py as _build_py

pybind11_include_dir = pybind11.get_include()


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
    version="0.1",
    description="Python Package with C++ Extension",
    ext_modules=ext_modules,
    cmdclass={"build_py": build_py},
)
