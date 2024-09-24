from conan import ConanFile

class Nexus(ConanFile):
    name = "nexus"
    version = "0.0.0"
    generators = "PkgConfigDeps"

    def requirements(self):
        self.requires("spdlog/1.14.1")
        self.requires("fmt/11.0.2", override=True)
        self.requires("flatbuffers/24.3.25")
        self.requires("stduuid/1.2.3")
        self.requires("abseil/20240722.0")

    def build_requirements(self):
        self.test_requires("gtest/1.15.0")
        self.test_requires("benchmark/1.9.0")