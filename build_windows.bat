@echo off

if not "%VCINSTALLDIR%" == "" goto :build

:: Try vswhere to locate VS installation
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath 2^>nul`) do (
	set "VS_PATH=%%i"
	goto :found_vs
)

:: Try to find VS 2022 (v17) / VS 2026 (v18) in common paths
for %%v in (18 2022 17) do (
	for %%e in (Enterprise Professional Community BuildTools) do (
		if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%%v\%%e\VC\Auxiliary\Build\vcvarsall.bat" (
			set "VS_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\%%v\%%e"
			goto :found_vs
		)
		if exist "%ProgramFiles%\Microsoft Visual Studio\%%v\%%e\VC\Auxiliary\Build\vcvarsall.bat" (
			set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\%%v\%%e"
			goto :found_vs
		)
	)
)

echo Error: Visual Studio 2022+ with "Desktop development with C++" workload not found.
echo Please run this script from an "x64_x86 Cross Tools Command Prompt" instead.
exit /b 1

:found_vs
echo Found VS at: %VS_PATH%
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
if %errorlevel% neq 0 (
	echo Error: Failed to initialize VS environment.
	exit /b %errorlevel%
)

:build
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
