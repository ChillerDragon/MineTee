+ MineTee README [![Circle CI](https://circleci.com/gh/CytraL/MineTee.svg?style=svg)](https://circleci.com/gh/CytraL/MineTee)
=============================================
MineTee its a mod for the game Teeworlds created and maintained by unsigned char*.

- Compile with MinGW
	If you get the error "'_hypot' was not declared in this scope..." you will need edit the file '\include\math.h'.
	Change the line:
	```c++
	{ return (float)(_hypot (x, y)); }"
	```
	To
	```c++
	{ return (float)(hypot (x, y)); }"
	```


	
	
+ TEEWORLDS README
=============================================
Copyright (c) 2011 Magnus Auvinen


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.


Please visit http://www.teeworlds.com for up-to-date information about 
the game, including new versions, custom maps and much more.
