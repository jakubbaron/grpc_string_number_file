#!/bin/bash

test_file_name="test.img"
test_server_file_name="server_storage/test.img"

### Setup ###
echo "Generaing 1G file..."
mkfile -n 1G $test_file_name
echo "Generated file "$test_file_name
ls -l $test_file_name

mkdir server_storage

make
./challenge_server & 
sleep 1
./challenge_client 
sleep 5
$(ps aux | grep -ie challenge_server | grep -v grep | awk '{print "kill " $2}')

diff -s $test_file_name $test_server_file_name

###Cleaning up###
rm $test_file_name
rm $test_server_file_name
make clean
rm -rf server_storage
