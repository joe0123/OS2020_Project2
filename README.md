# 2020 OS Project 2 - Synchronous Virtual Device

## Group 23 Members

* B06902017 趙允祥
* B06902105 吳吉加
* B06902047 陳彥
* B06902111 林慶珠
* B06902113 柯柏丞
* B06902125 黃柏瑋

## Task Description
https://hackmd.io/@buB4b8JxReG7RX6wHM9Iwg/HkeM7wt58

## Report
https://github.com/joe0123/OS2020_Project2/blob/master/report.pdf

## Kernel Version

**4.14.25**

## Usage

**This is the source code of operating system project2.**

>* ```./master_device``` : the device moudule for master server
>* ```./slave_device```  : the device moudule for slave client
>* ```./ksocket```  : the device moudule including the funtions used for kernel socket
>* ```./user_program``` : the user program "master" and "slave"
>

* Setup
```bash
git clone https://github.com/joe0123/OS2020_Project2.git
cd OS2020_Project2/
sudo ./init.sh
sudo ./compile.sh
cd user_program/
```
Note that our input and output files are too large to be uploaded to github. Therefore, they have to be downloaded by `./init.sh`.

* Run
```bash
# run commands on two terminals
sudo ./master 1 0.in mmap
sudo ./slave 1 0.out mmap 127.0.0.1
```
```
# or run bash scripts instead
./run_master1.sh mmap
./run_slave1.sh mmap
```

* It should generate a file `0.out` without any error.
