@echo off

rem  **************************************************
ren  INFORMATION
rem  **************************************************
rem  This is for launching 4coder after a build.
rem  The runnable 4ed.exe is in the 4cc\code\current_dist_super_x64\4coder\
rem  for some reason. The 4ed.exe located in 4cc\code\build will not run
rem  because it does not have access the the fallback font in that directory.

start .\current_dist_super_x64\4coder\4ed.exe
