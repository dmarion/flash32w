flash32w
========

Flash tool for STM32W based boards. Currently supports only boards with
STM32F based USB-to-Serial device (i.e. STM32W-RFCKIT boards).

Features: 

* Flashing bootloader into STM32F USB-to-Serial interface
* Flashing firmware to STM32W
* Device information
* STM32W Memory Dump

Due to poor implementation of CDC-ACM class in closed-source firmware, this
tool is using libsub to directly communicate with SRM32F USB-to-Serial 
instead of using operating system CDC-ACM Class driver.
This allows flashing on operating systems where use of original flash utility
is not possible (Mac OS X, FreeBSD).

Requirements
------------

It should work on any system which support [libusb](http://http://www.libusb.org).
In addition recent [cmake](http://www.cmake.org) is needed.

Building
--------

	$ cmake .
	-- The C compiler identification is GNU 4.2.1
	-- The CXX compiler identification is Clang 4.0.0
	-- Checking whether C compiler has -isysroot
	-- Checking whether C compiler has -isysroot - yes
	-- Checking whether C compiler supports OSX deployment target flag
	-- Checking whether C compiler supports OSX deployment target flag - yes
	-- Check for working C compiler: /usr/bin/gcc
	-- Check for working C compiler: /usr/bin/gcc -- works
	-- Detecting C compiler ABI info
	-- Detecting C compiler ABI info - done
	-- Check for working CXX compiler: /usr/bin/c++
	-- Check for working CXX compiler: /usr/bin/c++ -- works
	-- Detecting CXX compiler ABI info
	-- Detecting CXX compiler ABI info - done
	-- Found libusb-1.0:
	--  - Includes: /usr/local/include/libusb-1.0
	--  - Libraries: /usr/local/lib/libusb-1.0.dylib
	-- Configuring done
	-- Generating done
	-- Build files have been written to: /data/src/flash32w/1
	$ make
	Scanning dependencies of target flash32w
	[ 25%] Building C object CMakeFiles/flash32w.dir/main.c.o
	[ 50%] Building C object CMakeFiles/flash32w.dir/stm32w.c.o
	[ 75%] Building C object CMakeFiles/flash32w.dir/stm32f.c.o
	[100%] Building C object CMakeFiles/flash32w.dir/stm32f_usb.c.o

Known Issues
------------

In some cases burning of STM32F firmware fails when tool request switch to
bootloader mode, but in that case device will stay in bootloader mode so 2nd
attempt will typically succeed.

Obtaining STM32F USB-to-Serial Firmware
---------------------------------------

STM32F USB-to-Serial firmware is closed source firmware provided together with 
`stm32w_flasher` tool, There are 3 versions available from different software 
packages:

* Version 1.0.1 (8760 bytes) - This version in most cases works with CDC-ACM drivers on different OS
* Version 2.0.4 (25168 bytes) - Implements USB composite devices, not recognized by default CDC-ACM driver in most cases
* Version 2.0.6 (28276 bytes) - Implements USB composite devices, not recognized by default CDC-ACM driver in most cases

License
-------

This code is published under BSD 2-Clause license.

