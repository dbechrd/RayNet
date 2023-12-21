@echo off
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Release\capnp.exe        C:\Users\User\Documents\Development\RayNet\bin\capnp\capnp.exe /y
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Release\capnpc-c++.exe   C:\Users\User\Documents\Development\RayNet\bin\capnp\capnpc-c++.exe /y
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Release\capnpc-capnp.exe C:\Users\User\Documents\Development\RayNet\bin\capnp\capnpc-capnp.exe /y

xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Debug\capnp.lib        C:\Users\User\Documents\Development\RayNet\lib\capnp_d.lib /y
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Release\capnp.lib      C:\Users\User\Documents\Development\RayNet\lib\capnp.lib /y
rem xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Debug\capnpc.lib       C:\Users\User\Documents\Development\RayNet\lib\capnpc_d.lib /y
rem xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Release\capnpc.lib     C:\Users\User\Documents\Development\RayNet\lib\capnpc.lib /y
rem xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Debug\capnp-json.lib   C:\Users\User\Documents\Development\RayNet\lib\capnp-json_d.lib /y
rem xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\Release\capnp-json.lib C:\Users\User\Documents\Development\RayNet\lib\capnp-json.lib /y
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\kj\Debug\kj.lib              C:\Users\User\Documents\Development\RayNet\lib\kj_d.lib /y
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\kj\Release\kj.lib            C:\Users\User\Documents\Development\RayNet\lib\kj.lib /y

xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\capnp\*.h C:\Users\User\Documents\Development\RayNet\include\capnp /y
xcopy C:\Users\User\Documents\Development\_libs\capnproto\c++\src\kj\*.h    C:\Users\User\Documents\Development\RayNet\include\kj /y



