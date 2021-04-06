# EByte E32 SX1276 for the Raspberry Pi

See this [Blog Post](https://lloydrochester.com/post/hardware/e32-sx1276-lora/) for details. This repository has the source which requres autotools to build. If you just want to build the software and run it I recommend just getting the tarball below that is built from this repository source.

```
wget http://lloydrochester.com/code/e32-1.1.tar.gz
tar zxf e32-1.1.tar.gz
cd e32-1.1
./configure
make
./src/e32 --help
```

## Building the distribution

If you don't want the tarball to install you can build the autotools project with

```
./autogen.sh
./configure
make
```
