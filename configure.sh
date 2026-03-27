conan install . --output-folder=debug --build=missing -s compiler.cppstd=20 -s build_type=Debug
meson setup --native-file debug/conan_meson_native.ini debug

conan install . --output-folder=release --build=missing -s compiler.cppstd=20 -s build_type=Release
meson setup --native-file release/conan_meson_native.ini release

ln -sf release/compile_commands.json
