@echo off
REM Change to C: Drive
C:
REM Change to Arm DS directory
cd "C:\Program Files\Arm\Development Studio 2022.2\bin"
REM Attempt to check out a license
armlmdiag -f compiler5 -v 5.9 -q
REM Pause to show result
pause;
