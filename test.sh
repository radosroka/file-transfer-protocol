#!/bin/sh

# author: xsroka00
# 		: Radovan Sroka

echo "Compile ALL"
make

PORT_NUM=15000

echo "Run server instance"
./server -p $PORT_NUM &
 

echo "Uploading of file"
./client -h eva.fit.vutbr.cz -p $PORT_NUM -u test_file
 
#pokus o stiahnutie suboru test_data zo serveru
echo "Downloading of file"
./client -h eva.fit.vutbr.cz -p $PORT_NUM -d test_file
 
#pokus o odoslanie neexistujuceho suboru
#klient sa ukonci chybou
echo "Downloading of non-existing file"
./client -h eva.fit.vutbr.cz -p $PORT_NUM -u unknown
 
echo "TERMINATING"
kill $!
