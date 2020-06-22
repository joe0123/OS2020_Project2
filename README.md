# 2020 OS Project 2 - Synchronous Virtual Device

## Member

* B06902017 趙允祥
* B06902105 吳吉加
* B06902047 陳彥
* B06902111 林慶珠
* B06902113 柯柏丞
* B06902125 黃柏瑋

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
git clone https://github.com/hc07180011/OS2020-Project2-Group23.git
cd OS2020-Project2-Group23/
sudo ./compile.sh
cd user_program/
dd if=/dev/urandom of=0.in bs=64M count=1
```

* Run
```bash
# on two terminal
sudo ./master 0.in mmap
sudo ./slave 0.out mmap 127.0.0.1
```

* It should generate a file ```0.out``` without any error.

## Report
