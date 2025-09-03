@echo off
REM This script generates a Visual Studio 2022 solution for the cfast project.

REM Change the current directory to the location of the script.
REM cd /d "%~dp0"

REM Define the source and build directories.
set SOURCE_DIR=cfast
set BUILD_DIR=%SOURCE_DIR%/build

echo Generating Visual Studio solution in %BUILD_DIR%...

REM Generate the Visual Studio solution for 64-bit builds.
REM -- No need to specify the full path to the VS installation --
REM cmake -B %BUILD_DIR% -S %SOURCE_DIR% -G "Visual Studio 17 2022" -A x64
cmake -B %BUILD_DIR% -S %SOURCE_DIR% -G "Visual Studio 17 2022" -A x64
REM cmake -G "Visual Studio 17 2022" -A x64 ..

REM Generate the Visual Studio solution for 64-bit builds, specifying the Clang toolset.
REM cmake -B %BUILD_DIR% -S %SOURCE_DIR% -G "Visual Studio 17 2022" -A x64 -T "ClangCL"
REM cmake --build . --config Debug

REM Check if cmake was successful.
if %errorlevel% neq 0 (
    echo.
    echo CMake generation failed.
    pause
    exit /b %errorlevel%
)

echo.
echo Project generation complete.
echo You can now open the solution file: %BUILD_DIR%/cfast.sln
pause
