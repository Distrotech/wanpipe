@echo off

set CURRENT_DIR=%cd%
set SANG_WP_DEVEL=D:\development_trunk

set SANG_WARNING_LEVEL=/W3 /WX
#set SANG_WARNING_LEVEL=/W2

copy Makefile.Windows makefile
if %errorlevel% NEQ 0 goto failure

build -c -w
if %errorlevel% NEQ 0 goto failure

:ok
echo ********************
echo build was successful
echo ********************
goto end

:failure
echo !!!!!!!!!!!!!!!!!!!!!!!
echo build was UN-successful
echo !!!!!!!!!!!!!!!!!!!!!!!

:end
echo End
