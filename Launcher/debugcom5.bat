set WIILOAD=COM5
C://devkitPro//devkitPPC//bin//wiiload boot.elf
start C://devkitPro//insight//bin//powerpc-eabi-insight.exe boot.elf
start C://devkitPro//insight//bin//powerpc-eabi-gdb.exe boot.elf target remote COM5
