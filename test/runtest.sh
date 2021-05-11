#!/bin/bash

rm keydb/*
rmdir keydb
mkdir keydb

#make clean
#qmake 
make

./test
