@echo off

if not exist build\genesis.exe call build

pushd build
call genesis.exe
popd
