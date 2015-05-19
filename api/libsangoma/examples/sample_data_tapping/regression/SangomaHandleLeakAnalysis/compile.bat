@echo off

set SANG_WP_DEVEL=c:\development-git
echo SANG_WP_DEVEL is %SANG_WP_DEVEL%

echo _BUILDARCH is %_BUILDARCH%

if %_BUILDARCH%==AMD64 (set OBJ_DIR=objchk_wnet_amd64\amd64) else (set OBJ_DIR=objchk_wxp_x86\i386)

echo OBJ_DIR is defined as %OBJ_DIR%

if %_BUILDARCH%==x86	(set SANG_ARCH=-D__SANGx86__)
if %_BUILDARCH%==AMD64	(set SANG_ARCH=-D__x86_64__)

echo SANG_ARCH is defined as %SANG_ARCH%

rem pause

build /g
if errorlevel 1 goto failure

:ok
rem if we got here, there was no errors
echo ******************************************
echo build was successful
echo ******************************************
goto end

:failure
rem
echo !!!!!!!!!!!!!!!!!!!!!!!
echo build was UN-successful
echo !!!!!!!!!!!!!!!!!!!!!!!

:end
echo End
