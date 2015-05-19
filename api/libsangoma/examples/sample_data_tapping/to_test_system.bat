@echo off

set TEST_SYSTEM=\\%1%
echo TEST_SYSTEM is: %TEST_SYSTEM%

echo _BUILDARCH is: %_BUILDARCH%

if %_BUILDARCH%==AMD64 (set OBJ_DIR=objchk_wnet_amd64\amd64) else (set OBJ_DIR=objchk_wxp_x86\i386)
if %_BUILDARCH%==AMD64 (set DEST_FILE=%TEST_SYSTEM%\wanpipe\test_programs\g3ti_api_x64.exe) else (set DEST_FILE=%TEST_SYSTEM%\wanpipe\test_programs\g3ti_api_32.exe)

rem set DEST_FILE=%TEST_SYSTEM%\wanpipe\test_programs\g3ti_api.exe
rem set OBJ_DIR=temp

echo OBJ_DIR is: %OBJ_DIR%
echo DEST_FILE is: %DEST_FILE%

copy %OBJ_DIR%\g3ti_api.exe	%DEST_FILE%
if %errorlevel% NEQ 0 goto failure

echo **********************************************************
echo Files were copied to the Test System '%1'.
echo **********************************************************
goto end

:failure
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo failed to copy all files to the Test System '%1'
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

:end
