---------------------------------------------------------------------
+ MINETEE README [![Circle CI](https://circleci.com/gh/CytraL/MineTee.svg?style=svg)](https://circleci.com/gh/CytraL/MineTee)
---------------------------------------------------------------------
Copyright (c) 2016 Alexandre Díaz


This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


- MinGW Compilation issues

	If you get the error "'_hypot' was not declared in this scope..." you will need edit the file 'C:\MinGW\include\math.h'.
	Change the line:
	```c++
	{ return (float)(_hypot (x, y)); }
	```
	To...
	```c++
	{ return (float)(hypot (x, y)); }
	```
	*** More details here: http://stackoverflow.com/a/29489843



---------------------------------------------------------------------
+ TEEWORLDS README
---------------------------------------------------------------------

Copyright (c) 2011 Magnus Auvinen


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.


Please visit http://www.teeworlds.com for up-to-date information about 
the game, including new versions, custom maps and much more.
