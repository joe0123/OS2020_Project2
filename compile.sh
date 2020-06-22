#!/usr/bin/env bash
cd ./ksocket
if [ $1="bonus" ]
then
    make CFLAGS=-DBONUS
else
make
fi
insmod ksocket.ko
cd ../master_device
make
insmod master_device.ko
cd ../slave_device
make
insmod slave_device.ko
cd ../user_program
make


