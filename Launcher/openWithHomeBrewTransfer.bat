@echo off
if "%1" == "" goto error

set WIILOAD=tcp:192.168.1.3

:again
rem if %1 is blank, we are finished

if "%1" == "" goto end
echo.
echo Sending file %1...
E:\PMdev\devkitPro\devkitPPC\bin\wiiload.exe %1

rem - type  %1
rem - shift the arguments and examine %1 again

shift
goto again

:error
echo missing argument!
echo usage  view file1.txt view2.txt ...

:end
echo.
echo Done.
PAUSE