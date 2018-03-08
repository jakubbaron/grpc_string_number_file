test_file_name="test.img"
test_server_file_name="server_storage/test.img"
if[[ -f "/sbin/md5" ]] md5="/sbin/md5"
if[[ -f "/usr/bin/md5_sum" ]] md5="/usr/bin/md5_sum"

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
ps aux | grep -ie challenge_server | awk '{print "kill -9 " $2}'

$md5 $test_file_name
$md5 $test_server_file_name
diff -s $test_file_name $test_server_file_name

###Cleaning up###
rm $test_file_name
rm $test_server_file_name
make clean
rm -rf server_storage
