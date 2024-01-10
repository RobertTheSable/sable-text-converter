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
    options = {
        "build_tests": [True, False], 
        "build_asar": [True, False], 
        "use_system_icu": [True, False]
    }
    default_options = {
        "build_tests": False, 
        "build_asar": True, 
        "use_system_icu": False
    }
    
    def layout(self):    
        cmake_layout(self)
        
    def requirements(self):
        if self.options.build_tests:
            self.requires("catch2/2.13.9")
        if not self.options.use_system_icu:
            self.requires("icu/73.2")

    def generate(self):
        cmake = CMakeDeps(self)
        cmake.generate()
        tc = CMakeToolchain(self)
        if self.options.build_tests:
            tc.variables["SABLE_BUILD_TESTS"] = "ON"
            tc.variables["SABLE_BUILD_MAIN"] = "OFF"
        else:
            tc.variables["SABLE_BUILD_TESTS"] = "OFF"
            tc.variables["SABLE_BUILD_MAIN"] = "ON"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
