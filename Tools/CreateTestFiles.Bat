@echo off
setLocal EnableDelayedExpansion
set arg=%1

FOR /L %%i IN (1, 1, %arg%) DO (
  set FILENAME=Setup!RANDOM!.exe
  copy /b Setup.exe !FILENAME!
)

pause 