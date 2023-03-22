from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout


class SableTextConverterConan(ConanFile):
    name = "sable-text-converter"
    version = "0.1.0"
    license = "MIT License"
    author = "Robert The Sable <robertthesable@gmail.com>"
    url = "https://github.com/RobertTheSable/sable-text-converter"
    description = "Confiugrable text converter for SNES games."
    settings = "os", "compiler", "build_type", "arch"
    requires = "yaml-cpp/0.6.3", "cxxopts/2.2.0"
    options = {"build_tests": [True, False], "build_asar": [True, False], "system_icu": [True, False]}
    default_options = {"build_tests": False, "build_asar": True}
    
    def layout(self):    
        cmake_layout(self)

    def generate(self):
        cmake = CMakeDeps(self)
        cmake.generate()
        tc = CMakeToolchain(self)
        if self.options.build_tests:
            self.requires = "catch2/2.13.9"
            tc.variables["SABLE_BUILD_TESTS"] = "ON"
            tc.variables["SABLE_BUILD_MAIN"] = "OFF"
        else:
            tc.variables["SABLE_BUILD_TESTS"] = "OFF"
            tc.variables["SABLE_BUILD_MAIN"] = "ON"
        if self.options.system_icu:
            self.requires = {"boost/1.71.0", "icu/66.1"}
        tc.variables["QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP"] = "Off"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
