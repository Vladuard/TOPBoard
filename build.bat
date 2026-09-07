@echo off
echo Compiling TOPBoard...
g++ -o marker.exe main.cpp -lgdi32 -luser32 -lcomdlg32 -static
echo Done!
pause