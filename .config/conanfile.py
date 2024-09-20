from conan import ConanFile

class Nexus(ConanFile):
    name = "nexus"
    version = "0.0.0"
    generators = "PkgConfigDeps"

    def requirements(self):
        self.requires("spdlog/1.14.1")
        self.requires("fmt/11.0.2", override=True)
        self.requires("capnproto/1.0.2")
        self.requires("stduuid/1.2.3")

    def build_requirements(self):
        self.test_requires("gtest/1.15.0")
        self.test_requires("benchmark/1.9.0")