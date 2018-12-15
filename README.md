#  ZeldaOS 

`Written in C&inline assembly from scratch, ZeldaOS is an unix-like 32bit monolithic kernel which is designed to comform to POSIX.1 interface. At present, it's only able to run on any x86 and x86_64 processors. The name Zelda is from Nintendo video game: The Legend of Zelda`

### How to Build The Project ?
To build the project, we need a 64bit Linux host (I used a CentOS 7.3) and GCC suite(my GCC version is 4.8.5, other versioned gcc are supposed to work), in order to  package a bootable ISO image, I also install rpm package: `#yum install -y xorriso`. the compilation tools will automatically generate 32bit elf objects with [Makefile and Linker script](https://github.com/chillancezen/ZeldaOS/tree/master/mk)


To build the image and ISO image(imagine the project top directory is `/root/ZeldaOS/`):
```
#ZELDA=/root/ZeldaOS/ make runtime_install
#ZELDA=/root/ZeldaOS/ make app_install
#ZELDA=/root/ZeldaOS/ make drive
#ZELDA=/root/ZeldaOS/ make
```
To clean the built objects:
```
#ZELDA=/root/ZeldaOS/ make runtime_clean
#ZELDA=/root/ZeldaOS/ make app_clean
#ZELDA=/root/ZeldaOS/ make clean
```

### How to Launch the OS/kernel ?
There are two ways to launch the ZeldaOS: 
#### (a) Launch ZeldaOS via the Zelda.bin raw kernel image.
this is the most usual way I did when I was debuging the code. you need to install qemu software beforehand, a typical command is given like:
```
#/usr/bin/qemu-system-x86_64 -serial tcp::4444,server -m 3024 -kernel Zelda.bin \
-monitor null -nographic -vnc :100 -netdev tap,id=demonet0,ifname=demotap0,script=no,downscript=no \
-device virtio-net-pci,netdev=demonet0,mac=52:53:54:55:56:00 \
-netdev tap,id=demonet1,ifname=demotap1,script=no,downscript=no \
-device virtio-net-pci,netdev=demonet1 -gdb tcp::5070
```
you can specify more different `-serial` parameter([qemu mannual](https://manpages.debian.org/testing/qemu-system-x86/qemu-system-x86_64.1.en.html)) to observe the output or input. right here you can use the shell by telnet to local qemu serial endpoint:
```
#telnet localhost 4444
....(omitted)
Welcome to ZeldaOS Version 0.1
Copyright (c) 2018 Jie Zheng [at] VMware

[Link@Hyrule.kingdom /home/zelda]# /usr/bin/uname
sysname  : ZeldaOS
nodename : Hyrule
release  : The Brave
version  : 0.1
machine  : i686
domain   : kingdom
[Link@Hyrule.kingdom /home/zelda]#
```
#### (b) Launch ZeldaOS via the Zelda.iso bootable image or hard drive.
In this case the kernel is booted by GRUB multiboot. you can burn the `Zelda.iso` into a udisk drive or hard drive from which you can boot the kernel.
we have the splash window when the kernel boots(grub draws it):
![image_of_splash](https://raw.githubusercontent.com/chillancezen/misc/master/image/zelda_os_splash.png)

When the kernel is fully ready, the `default console` (which you can navigate to by `Alt+F1`) is displayed, there are other 6  consoles initiated by [`/usr/bin/userland_init`](https://github.com/chillancezen/ZeldaOS/blob/master/application/userland_init/etc/userland.init), you can switch the console by `Alt+F2 ... Alt+F7`, you will observe console as below:

![image_of_console0](https://raw.githubusercontent.com/chillancezen/misc/master/image/zelda_os_console0.png)
