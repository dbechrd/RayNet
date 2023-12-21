@echo off
capnp convert binary:json ../../src/common/meta/resource_library.capnp ResourceLibrary < resource_library.bin > resource_library.json

