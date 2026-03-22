# mango-frunk, an e-ink display that shows some system stats.


## Running

If you have an AppImage build, just copy it to a SteamOS device and run it like:
* `./mango-frunk*.AppImage --install`

That it will install and run itself as a user service. Make sure bluetooth is
enabled on the device and then if you bring a panel nearby it should connect.

You may need to hit the reset button on the back of the panel once you have it
next to the SteamOS host machine in case it's already connected somewhere else.


## Building Firmware

1. Install [PlatformIO](https://platformio.org) for your system.
2. Navigate a terminal to the `./firmware` directory of this repo.
3. Run `pio run -t upload`
    * You need to have an ESP32-S3 Feather connected and it should build and flash.


## Building Service

If you have Qt installed you should be able to open the cmake project in Qt Creator,
but for distributable linux builds it's easiest to follow this process on SteamOS.

1. `distrobox create --pull --image archlinux:base-devel --name mango-frunk`
2. `distrobox enter mango-frunk`
    * **NOTE:** Following commands are all ran within the distrobox!
3. `sudo pacman -Syu base-devel wget cmake git fuse2 qt6`
4. `./scripts/build.sh deploy`

That should produce an AppImage for you in the `./dist-linux-x86_64` folder!


## Building Using Custom Container

You can setup a better container for wider platform support on the builds by
using the included `Containerfile`.

1. `podman build -t qt69-builder -f ./Containerfile`
2. `distrobox create --image qt69-builder --name qt69`
3. `distrobox enter qt69`
4. `./scripts/build.sh deploy`

This container uses an older version of Ubuntu and Qt 6.9 which should let us
build AppImages that will work on a wide range of modern systems.
