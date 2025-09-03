REM ==============================================================================
REM build.bat - Windows batch file for quick builds (create this file)
REM ==============================================================================

@echo off
setlocal

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with Visual Studio 2022
echo Configuring project...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %errorlevel% neq 0 goto error

:: Build Debug
echo.
echo Building Debug configuration...
cmake --build . --config Debug
if %errorlevel% neq 0 goto error

:: Build Release
echo.
echo Building Release configuration...
cmake --build . --config Release
if %errorlevel% neq 0 goto error

:: Run tests
echo.
echo Running tests...
ctest -C Debug
if %errorlevel% neq 0 goto error

echo.
echo Build successful!
echo Executables are in: build\bin\
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
cd ..
endlocal
