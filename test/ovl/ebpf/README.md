# Xcluster/ovl - eBPF

Experiments and tests with
[XDP](https://en.wikipedia.org/wiki/Express_Data_Path) and
[eBPF](https://ebpf.io/).

## Prerequisites

We will use the `xcluster` kernel, not the kernel you have on your
computer, so it must be built locally;

```
xc env | grep __kver     # kernel version
xc env | grep KERNELDIR  # kernel source unpacked here
xc kernel_build
```

## Build latest QEMU (for IGB support)
```
mkdir build
cd build
../configure --target-list=x86_64-softmmu
make -j$(nproc)
make DESTDIR=$PWD/sys install
```
