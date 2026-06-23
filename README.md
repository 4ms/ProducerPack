# 4ms Producer Pack for MetaModule and VCV Rack

Helpful producer utilities.

Compiles as a VCV Rack plugin or a MetaModule plugin.

[Read the user manual here](doc/Producer Pack User Manual.md)


## Building:

Get the repo and clone submodules:

```bash
git clone https://github.com/4ms/ProducerPack

cd ProducerPack
git submodule update --init --recursive
```

### Build the MetaModule plugin:

_Requires arm-none-eabi-g++ toolchain v12.2 or v12.3 on your PATH (newer versions will not work, must be these versions only)._

```bash
cmake --fresh -B build
cmake --build build
```

If you need to specify the path to the arm toolchain (v12.2.x or v12.3.x is REQUIRED):
```bash
cmake --fresh -B build -DTOOLCHAIN_BASE_DIR=~/bin/arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi/bin
cmake --build build
```

The MM plugin will be in `metamodule-plugins/`

### Build the VCV Rack plugin:

See [VCV Rack development environment](https://vcvrack.com/manual/Building) for pre-requisites.

```bash
make dep
make install
```


