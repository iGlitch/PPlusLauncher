@echo off
Title Patch Generator

::set output path
ren paths.txt paths.tmp
setlocal enabledelayedexpansion
set lines=0&set hax=%~dp0
for /f "tokens=*" %%x in (paths.tmp) do (set /a lines+=1 & set var[!lines!]=%%x)
echo %var[1]%>>paths.txt&echo %var[2]%>>paths.txt&echo %hax%output>>paths.txt&echo %var[4]%>>paths.txt&echo %var[5]%>>paths.txt
del paths.tmp

::run Patcher.exe with paths
Patcher.exe paths.txt
echo:&echo removing empty directories
ROBOCOPY output output /S /MOVE >nul

::get name and add to archive
color b&echo:&echo Enter build abbreviation (examples: pm te xp lt):
set /P "xx="
color 6&echo:&echo Enter base build version (examples: 1.0 1.11 2.6 3.01 3.5):
set /P "base="
color d&echo:&echo Enter new build version (examples: 1.1 2.5 2.64 3.02 3.6):
set /P "patch="
:7zip
color 8
call "C:\Program Files\7-Zip\7z" a -mx=9 -m1=LZMA -ms=off %xx%update-%base%-%patch%.7z .\output\. -sdel || goto :choice
color a
rmdir output

::get archive file size and create variable for update.xml
call :filesize "%xx%update-%base%-%patch%.7z"
:filesize
set size=%~z1

::get md5 cheksum and create varible for update.xml
set "MD5="
for /f "skip=1 Delims=" %%# in ('certutil -hashfile "%xx%update-%base%-%patch%.7z" MD5') do If not defined MD5 Set MD5=%%#
Set MD5=%MD5: =%
echo MD5 Checksum: %MD5%

::get url from paths.txt for update.xml
for /f "skip=4 delims=" %%x in (paths.txt) do set "url=%%x"
echo server: %url%

::create update.xml with variables
echo generating update.xml
(
  echo ^<root^>
  echo 	^<projectm^>
  echo 		^<update updateVersion="%patch%" length="%size%" md5="%md5%"^>
  echo 			^<baseVersionsSupported^>
  echo 				^<baseVersion^>%base%^</baseVersion^>
  echo 			^</baseVersionsSupported^>
  echo 			^<url^>%url%%xx%update-%base%-%patch%.7z^</url^>
  echo 		^</update^>
  echo 	^</projectm^>
  echo 	^<launcher^>
  echo 		^<update updateVersion="" length="" md5=""^>
  echo 		^<baseVersionsSupported^>
  echo 			^<baseVersion^>*^</baseVersion^>
  echo 		^</baseVersionsSupported^>
  echo 		^<url^>^</url^>
  echo 		^</update^>
  echo 	^</launcher^>
  echo 	^<news^>^</news^>
  echo ^</root^>
) > update.xml
echo:&echo Finished^^!

::end
echo press any key to exit . . .&pause >nul
start "" notepad.exe update.xml
exit

::7zip install
:choice
color c&echo:
set /P c=7zip installation not found, install 7zip now[Y/N]?
if /I "%c%" EQU "Y" goto :download
if /I "%c%" EQU "N" goto :end
goto :choice
:end
exit
:download
start https://www.7-zip.org/
echo 7-Zip now installed? & pause
goto :7zip