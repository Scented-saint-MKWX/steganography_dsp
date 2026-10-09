@echo off
REM build_host_tests.bat - Build and run host tests under Windows (MinGW/Clang/GCC)
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..

echo === Building Host Tests via Makefile ===
cd /d "%PROJECT_ROOT%\target"
make clean
make host
make host_stego

echo.
echo === Running Unit Test Suite ===
build_host\test_runner.exe

echo.
echo === Running Host Stego Execution ===
build_host\stego_host.exe

echo.
echo All host tests finished successfully!
