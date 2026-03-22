import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import get


class UServerCqlDriverConan(ConanFile):
    name = "userver-cql-driver"
    description = "CQL (Cassandra Query Language) driver for userver framework"
    license = "Apache-2.0"
    topics = ("userver", "cassandra", "cql", "driver")

    package_type = "static-library"
    settings = "os", "arch", "compiler", "build_type"

    exports_sources = ["CMakeLists.txt", "src/*", "include/*", "cmake/*"]

    def requirements(self):
        self.requires("userver/[^2.15]")
        self.requires("lz4/[>=1.9]")

    def validate(self):
        check_min_cppstd(self, 20)

    def layout(self):
        cmake_layout(self)

    def source(self):
        if not os.path.exists(
            os.path.join(self.source_folder, "CMakeLists.txt")
        ):
            get(
                self,
                **self.conan_data["sources"][self.version],
                strip_root=True,
            )

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["CASSANDRA_BUILD_SAMPLES"] = False
        tc.variables["CASSANDRA_BUILD_TESTS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "userver-cql-driver")
        self.cpp_info.set_property("cmake_target_name", "userver::cassandra")
        self.cpp_info.libs = ["userver-cql-driver"]
