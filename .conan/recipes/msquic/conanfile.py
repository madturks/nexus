from os import path
from shutil import rmtree
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.scm import Git


class msquicRecipe(ConanFile):
    name = "msquic"
    version = "2.4.5"
    package_type = "library"

    # Optional metadata
    license = "MIT"
    author = "Microsoft QUIC Team <quicdev@microsoft.com>"
    url = "https://github.com/microsoft/msquic"
    description = "Cross-platform, C implementation of the IETF QUIC protocol, exposed to C, C++, C# and Rust."
    topics = ("c", "cpp", "quic", "cross-platform", "networking", "protocol", "secure")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "tls_library": ["openssl", "openssl3", "schannel", "default"],
        "use_xdp": [True, False],
        "build_tools": [True, False],
        "build_test": [True, False],
        "build_perf": [True, False],
        "use_system_crypto": [True, False],
        "enable_logging": [True, False],
    }
    default_options = {
        "shared": True,
        "fPIC": True,
        "tls_library": "default",
        "use_xdp": False,
        "build_tools": False,
        "build_test": False,
        "build_perf": False,
        "use_system_crypto": True,
        "enable_logging": False,
    }

    exports_sources = "src/*"

    def requirements(self):
        self.requires("libnuma/2.0.16")
        self.requires("openssl/3.3.2")
        self.requires("zstd/1.5.6")
        self.requires("libelf/0.8.13")
        # Don't have following packages in conan-center.
        # self.requires("libz/4.8.12")

        if self.options.use_xdp:
            self.requires("libnl/3.9.0")
            self.requires("libbpf/1.3.0")
            # Don't have following packages in conan-center.
            # self.requires("libnl-route/3.x.x")
            # self.requires("libxdp/1.4.2")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def source(self):
        # if msquic path exists delete it
        if path.exists("src"):
            rmtree("src")
            
        git = Git(self)
        git.clone(url=self.url, target=".")
        git.checkout(commit=f"v{self.version}")
        git.run("submodule update --init --recursive")

    def layout(self):
        cmake_layout(self, src_folder="src")
    
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        variables={
            "QUIC_BUILD_TOOLS": self.options.build_tools,
            "QUIC_BUILD_TEST": self.options.build_test,
            "QUIC_BUILD_PERF": self.options.build_perf,
            "QUIC_USE_SYSTEM_LIBCRYPTO": self.options.use_system_crypto,
            "QUIC_ENABLE_LOGGING": self.options.enable_logging,
        }

        if self.options.tls_library != "default":
            if self.settings.os != "Windows" and self.options.tls_library == "schannel":
                raise Exception("Schannel is only supported on Windows")
            variables["QUIC_TLS"] = self.options.tls_library

        if self.options.tls_library == "default":
            if self.settings.os == "Windows":
                variables["QUIC_TLS"] = "schannel"
            else:
                variables["QUIC_TLS"] = "openssl3"

        cmake = CMake(self)
        cmake.configure(variables=variables)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["msquic"]
