from conan import ConanFile
from conan.tools.meson import MesonToolchain
from conan.tools.gnu import PkgConfigDeps
from conan.tools.env import VirtualBuildEnv
import os


class VipsBuildConan(ConanFile):
    name = "vips-build"
    version = "1.0"
    settings = "os", "arch", "compiler", "build_type"

    requires = (
        "glib/2.85.3",
        "pcre2/10.42",
        "libffi/3.4.4",
        "zlib/1.3.1",
        "bzip2/1.0.8",
        "libgettext/0.22",
        "expat/2.6.2",
    )

    def requirements(self):
        self.requires("libiconv/1.18", override=True)

    def build_requirements(self):
        self.tool_requires("meson/1.9.1")
        self.tool_requires("ninja/1.13.1")
        self.tool_requires("pkgconf/2.5.1")
        self.tool_requires("glib/2.85.3")
        self.tool_requires("libiconv/1.18", override=True)

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.build_context_activated = [
            dep.ref.name for dep in self.dependencies.build.values()]
        pc.build_context_folder = "build-pc"
        pc.generate()
        VirtualBuildEnv(self).generate()

        tc = MesonToolchain(self)

        iconv = self.dependencies["libiconv"]
        pkg = iconv.package_folder

        for incdir in iconv.cpp_info.includedirs:
            inc_path = os.path.join(pkg, incdir)
            tc.c_args.append(f"-I{inc_path}")
            tc.cpp_args.append(f"-I{inc_path}")

        for libdir in iconv.cpp_info.libdirs:
            abslib = os.path.join(pkg, libdir)
            tc.c_link_args.append(f"-L{abslib}")
            tc.cpp_link_args.append(f"-L{abslib}")
            # rpath-link helps Meson's link test in cross builds
            tc.c_link_args.append(f"-Wl,-rpath-link,{abslib}")
            tc.cpp_link_args.append(f"-Wl,-rpath-link,{abslib}")

        for lib in iconv.cpp_info.libs:
            tc.c_link_args.append(f"-l{lib}")
            tc.cpp_link_args.append(f"-l{lib}")

        tc.generate()
