@echo off
REM Change to C: Drive
C:
REM Change to DS-5 directory
if exist C:\intelFPGA\17.1\embedded\ds-5\bin (
    cd "C:\intelFPGA\17.1\embedded\ds-5\bin"
) else (
    cd "C:\Program Files\DS-5 v5.27.1\bin"
)
REM Attempt to check out a license
armlmdiag -f compiler5 -v 5.01 -q
REM Pause to show result
pause;
