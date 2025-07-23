@echo off

if not exist build\gen.exe call build

pushd build
call gen.exe
popd
