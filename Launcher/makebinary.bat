@C:\devkitPro\devkitPPC\bin\powerpc-eabi-gcc.exe -DASM -nostartfiles -nodefaultlibs -Wl,-Ttext,0x80001800 -o codehandler.elf codehandler.s
@C:\devkitPro\devkitPPC\bin\powerpc-eabi-strip.exe --strip-debug --strip-all --discard-all -o codehandler.elf -F elf32-powerpc codehandler.elf
@C:\devkitPro\devkitPPC\bin\powerpc-eabi-objcopy.exe -I elf32-powerpc -O binary codehandler.elf source\codehandler.bin
raw2c source\codehandler.bin

rem GCT files
rem raw2c "..\..\codes\ported\Clone Engine Codeset\RSBJ01.gct"
copy "..\..\codes\WiFi Clone Engine Codeset\RSBE01.gct" wRSBE01.gct
raw2c wRSBE01.gct
copy "..\..\codes\Clone Engine Codeset\RSBE01.gct"
raw2c RSBE01.gct

rem Files for disk loading
raw2c fst_J.bin
raw2c fst_U.bin

move codehandler.c source\DiskLaunch\codehandler.c
move codehandler.h source\DiskLaunch\codehandler.h
move wRSBE01.c source\DiskLaunch\wRSBE01.c
move wRSBE01.h source\DiskLaunch\wRSBE01.h
move RSBE01.c source\DiskLaunch\RSBE01.c
move RSBE01.h source\DiskLaunch\RSBE01.h
rem move RSBJ01.c source\DiskLaunch\RSBJ01.c
rem move RSBJ01.h source\DiskLaunch\RSBJ01.h
move fst_J.c source\Patching\fst_J.c
move fst_J.h source\Patching\fst_J.h
move fst_U.c source\Patching\fst_U.c
move fst_U.h source\Patching\fst_U.h
del *.elf
del source\*.bin
del *.gct