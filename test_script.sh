#!/bin/bash

test_file_name="test.img"
test_server_file_name="server_storage/test.img"
test_received_client_file_name="received_lorem_ipsum.img"

### Setup ###
echo "Generaing 1G file..."
dd if=/dev/zero of=$test_file_name count=1024 bs=1048576
mkfile -n 1G $test_file_name
echo "Generated file "$test_file_name
ls -l $test_file_name

make
./challenge_server &
sleep 1
./challenge_client
$(ps aux | grep -ie challenge_server | grep -v grep | awk '{print "kill " $2}')
sleep 3 #allow server to shut down, don't mix logs

echo ""
echo "Testing File sent Client -> Server"
diff -s $test_file_name $test_server_file_name
echo ""

echo ""
echo "Testing File requested by Client from Server"
diff -s "server_storage/lorem_ipsum.img" $test_received_client_file_name
echo ""

###Cleaning up###
echo "Cleaning up after tests"
rm $test_file_name
rm $test_server_file_name
rm $test_received_client_file_name
make clean
