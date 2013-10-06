@nmake DEBUG=1 DEBUG_MEM=1 DEBUG_LOG=1
if errorlevel 1 goto err
copy drc123.dll \temp\gcc\
:err