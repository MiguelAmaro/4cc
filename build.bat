@echo off

rem  **************************************************
ren  INFORMATION
rem  **************************************************
rem  This is for building 4coder easily.
 
pushd code
echo building 4coder...
start .\bin\package.bat
popd

