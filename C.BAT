gcc boxjump.c -o boxjump.exe -lallegro -lm -Wall -s -O3 -fdata-sections -ffunction-sections -Xlinker --gc-sections
exe2coff boxjump.exe
copy /B cwsdstub.exe+boxjump boxjump.exe
del boxjump