@echo off

 Copyright (C) 1994-2018 Altair Engineering, Inc.
 For more information, contact Altair at www.altair.com.

 This file is part of the PBS Professional("PBS Pro") software.

 Open Source License Information:

 PBS Pro is free software. You can redistribute it and/or modify it under the
 terms of the GNU Affero General Public License as published by the Free
 Software Foundation, either version 3 of the License, or (at your option) any
 later version.

 PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.
 See the GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 Commercial License Information:

 For a copy of the commercial license terms and conditions,
 go to: (http://www.pbspro.com/UserArea/agreement.html)
 or contact the Altair Legal Department.

 Altair’s dual-license business model allows companies, individuals, and
 organizations to create proprietary derivative works of PBS Pro and
 distribute them - whether embedded or bundled with other software -
 under a commercial license agreement.

 Use of Altair’s trademarks, including but not limited to "PBS™",
 "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 trademark licensing policies.


REM This script will generate msi installer using Wix toolset

REM Defining needed variables 

SET PBS_prefix=''
@setlocal enableextensions enabledelayedexpansion
set variable=%~dp0
if "x!variable:~-29!"=="xpbspro\win_configure\msi\wix\" (
	set variable=!variable:~0,-29!
) else (
@echo "Failed to parse PBS prefix location"
goto theend
)

SET PBS_prefix=!variable!

SET TOPDIR=%PBS_prefix%PBS

REM Calling wix compiler

candle Product.wxs temp\pbsproexec.wxs temp\pbsprohome.wxs -ext WixUtilExtension.dll -ext WixUIExtension -d"pbsproexec=%TOPDIR%\exec" -d"pbsprohome=%TOPDIR%\home"
@if NOT %ERRORLEVEL% == 0 goto theend

REM Caling Wix linker
light -ext WixUIExtension -ext WiXUtilExtension pbsproexec.wixobj Product.wixobj pbsprohome.wixobj -o PBSPro.msi -d"pbsproexec=%TOPDIR%\exec" -d"pbsprohome=%TOPDIR%\home"
@if NOT %ERRORLEVEL% == 0 goto theend


@echo Finished successfully..
exit /b

:theend
@echo "Error: while creating msi installer" 
exit /b
