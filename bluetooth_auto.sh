#!/bin/bash

devAddr="00:1D:A5:1D:0A:49"

checkIfBlueAlreadyConnected()
{
if [ -c "/dev/rfcomm0" ]; then
  echo "bluetooth connnected ok \n"
  exit 0
else
  echo "begin to search obd \n"
fi
}


#try direct connect, if bluetooth device already paired before, it will success
rfcomm connect 0 $devAddr 1 &
sleep 2
checkIfBlueAlreadyConnected


#try multiple times to pair&connect bluetooth
for i in 1 2 3 
do

echo "try $i times connect OBD"

#step1: pair, need input obd device name "V-LINK" in future instead of address
./bluePair.sh $devAddr 

sleep 1

#step2:connect and check result
rfcomm connect 0 $devAddr 1 &
sleep 2
checkIfBlueAlreadyConnected

done #end for loop

echo "after multiple times try, finnaly failed,pls check OBD power on"
exit 1
