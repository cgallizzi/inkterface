# Inkterface, an e-ink faceplate for your Steam Machine.


## Running

If you have an AppImage build, just copy it to a SteamOS device and run it like:
* `./inkterface.AppImage`

That it will install and run itself as a user service. Make sure bluetooth is
enabled on the device and then if you bring a panel nearby it should connect.

You may need to hit the reset button on the back of the panel once you have it
next to the SteamOS host machine in case it's already connected somewhere else.


## Design Docs

Planning out the UI and panel design is done in a Lunacy document you can find
in the `./design` folder.

Lunacy is an open source alternative to tools like Figma that can work entirely
offline.


## Building Firmware

1. Install [PlatformIO](https://platformio.org) for your system.
2. Navigate a terminal to the `./firmware` directory of this repo.
3. Run `pio run -t upload`
    * You need to have an ESP32-S3 Feather connected and it should build and flash.


## Building

If you have Qt installed you should be able to open the cmake project in Qt Creator
and that will should work for you!

For better distributable builds You can setup a container for wider platform
support using the included `Containerfile`.

1. `podman build -t qt69-builder -f ./Containerfile`
2. `distrobox create --image qt69-builder --name qt69`
3. `distrobox enter qt69`
4. `./scripts/build.sh deploy`

That should produce an AppImage for you in the `./dist-linux-x86_64` folder!

This container uses an older version of Ubuntu and Qt 6.9 which should let us
build AppImages that will work on a wide range of modern systems.
