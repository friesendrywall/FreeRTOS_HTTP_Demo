@echo off
cd ../web
call yarn build
cd ../firmware
myromfs web-source.txt