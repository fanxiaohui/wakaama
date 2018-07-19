#!/bin/bash


if [ -c "/dev/rfcomm0" ]; then
  echo "bluetooth already connnected \n"
  exit 0
fi

#try 3 times to pair&connect bluetooth
for i in 1 2 3 
do

echo "try $i times connect OBD"

#step1: pair, need input obd device name "V-LINK" in future instead of address
./bluePair.sh 00:1D:A5:1D:0A:49

sleep 1

#step2:connect
rfcomm connect 0 00:1D:A5:1D:0A:49 1 &

sleep 2

if [ -c "/dev/rfcomm0" ]; then
  echo "bluetooth connect ok\n"
  exit 0
else
  echo "bluetooth connect fail\n"
fi

done #end for loop

echo "after 3 times try, finnaly failed,pls check OBD power on"
exit 1
