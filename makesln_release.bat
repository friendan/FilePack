@echo off
cmake -B build_release -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release
