# EByte E32 SX1276 for the Raspberry Pi

See this [Blog Post](https://lloydrochester.com/post/hardware/e32-sx1276-lora/) for details.

This repository contains the source code, as well as, the source code to distribute the tool which requres GNU Autotools to build. If you just want to run the tool I recommend just getting the tarball below where you can build from source.

# Getting Started

```
wget http://lloydrochester.com/code/e32-1.2.tar.gz
tar zxf e32-1.2.tar.gz
cd e32-1.2
./configure
make
./src/e32 --help
```

## Building the distribution

If you don't want the tarball you could build using the GNU Autotools.

```
./autogen.sh # this creates the configure script and Makefiles
./configure
make
```
