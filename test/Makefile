
all:	clean copy test

copy:
	copy ..\dist\files work
	copy ..\dist\files\visplug work\visplug

test:
	test.cmd

clean:
	-del ..\dist\work\*.ini ..\dist\work\*.lst 2>nul