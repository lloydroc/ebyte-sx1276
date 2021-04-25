# EByte E32 SX1276 for the Raspberry Pi

See this [Blog Post](https://lloydrochester.com/post/hardware/e32-sx1276-lora/) for details.

This repository contains the source code, as well as, the source code to distribute the tool which requres GNU Autotools to build. If you just want to run the tool I recommend just getting the tarball below where you can build from source.

# Getting Started

We're going to assume you have 2 E32 Modules attached to two Raspberry PIs. Thus, one can transmit and the other receive and vice-versa. Details for each step in the [Blog Post](https://lloydrochester.com/post/hardware/e32-sx1276-lora/).

1. Wire up your E32 modules.
2. Using `raspi-config` configure your Serial Port, Unix groups and UART File Permissions.
3. Install the `e32` command line tool. See below.
4. Read the version and status from the `./e32 --status`. If this doesn't work the next one won't.
5. Do an end-to-end test to transmit from one and receive on the other. See below.

## Install the `e32` command line tool and get status

```
wget http://lloydrochester.com/code/e32-1.3.tar.gz
tar zxf e32-1.3.tar.gz
cd e32-1.3
./configure
make
sudo make install
e32 --help
e32 --status
```

## End-to-End test - Transmit from one E32 and receive on the other

Again we assume you have two E32 modules attached to two different Raspberry Pi modules.

Run `e32` on both at the same time, no options are needed. In one terminal type something and hit enter. This will transmit what was typed. On the other terminal you should see what you typed. By doing this what you typed when through the UART to one E32, was transmitted, received by the other E32, read out the other UART and was output onto the terminal. Now do this in the other direction.

# Advanced Features

The tool offers more than just taking input from a keyboard. It's meant to run as a daemon and run in the background. If, however, you don't run it as a daemon you can send files and/or save to a file.

When running as a daemon communication to and from the `e32` is via Unix Domain Socket. This allows other tools written an any language to communicate wirelessly by just sending and receiving from a socket. See the blog post for an example in Python.

## Building the distribution

If you don't want the tarball you could build using the GNU Autotools.

```
# clone this repo
./autogen.sh # this creates the configure script and Makefiles
./configure
make
```
