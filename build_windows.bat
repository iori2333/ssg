@echo off

if "%VCINSTALLDIR%" == "" (
	echo Error: The build must be run from within Visual Studio's `x64_x86 Cross Tools Command Prompt`.
	exit 1
)

:: Ensure submodules are initialized
git submodule update --init --recursive

:: Configure and build (CMake generates version header from git)
cmake -B build -S . -G "Ninja" ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DCMAKE_C_COMPILER=cl ^
	-DCMAKE_CXX_COMPILER=cl

if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build build --config Release
exit /b
