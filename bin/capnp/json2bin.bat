@echo off
capnp convert json:binary ../../src/common/meta/resource_library.capnp ResourceLibrary < resource_library.json > resource_library.bin

