# Inkterface, an e-ink faceplate for your Steam Machine.


## Required Hardware

* 1 x Adafruit ESP32 Feather with 2MB PSRAM
    * [Link to V1 Board, as shown in the assembly video and document!](https://www.adafruit.com/product/5477)
    * [Link to V2 Board, accidentally the only link previously!](https://www.adafruit.com/product/5400)
    * Both the V1 and V2 modules work, but they have different pinouts, make sure
      to check if you see the "V2" silkscreen on your board and choose the correct
      pinout sheet and environment when building the firmware!
* 1 x [Adafruit eInk Breakout Friend](https://www.adafruit.com/product/4224)
* 1 x [Adafruit 5.83" Monochrome eInk Panel](https://www.adafruit.com/product/6397)
* 1 x [LP803860 Battery](https://www.adafruit.com/product/2011)
* 13 x M2.5 x 5mm Pan Head Machine Screws
    * [McMaster](https://www.mcmaster.com/92000A103/)
    * [Amazon](https://www.amazon.com/dp/B0GC58C2V5)
* 4 x [1/4" x 1/4" x 3/16" Stepped Magnet SB443-OUT](https://www.kjmagnetics.com/sb443-out-neodymium-stepped-block-magnet)


## Assembly

Check out the video `./docs/Inkterface Assembly.mp4` for a quick assembly tutorial,
or if you'd prefer there is a PDF version too `./docs/Inkterface Assembly.pdf`.

**_Warning: The screws will thread themselves into the plastic, but be VERY gentle
as it is easy to strip the plastic away and you may need to re-print parts!_**

1. Print parts from `./cad` folder.
    * Individual parts are included as separate STEP files, but a combined file
      with all parts that is ready for printing on a Stratasys F370 build plate
      is also provided as `Inkterface - Print Plate.step`.
2. Attach the ESP32 Feather and eInk Breakout to the board plate layer using 4x screws.
    * Having them held in place helps with soldering the connections and sizing your wires.
3. Solder wires between the pins of the two boards.
    * MOSI/MISO/SCK/GND all connect directly as expected.
    * 3V3 from the feather connects to VIN on the breakout.
        * The 3V3 pin on the breakout is a 3V3 output.
    * For the **_V1_** feather connect these pins, like Feather -> Breakout:
        * PIN 6 -> ENA
        * PIN 9 -> BUSY
        * PIN 10 -> RST
        * PIN 11 -> SRCS
        * PIN 12 -> D/C
        * PIN 13 -> ECS
    * For the **_V2_** feather connect these pins, like Feather -> Breakout:
        * PIN 32 -> ENA
        * PIN 15 -> BUSY
        * PIN 33 -> RST
        * PIN 27 -> SRCS
        * PIN 12 -> D/C
        * PIN 13 -> ECS
    * **_NOTE:_** On both feather versions we connect to pins in the same physical
        positions, but they are different logical pins in the firmware.
4. Place the e-ink panel into the matching recess in the faceplate part.
5. Fit the two midplates in.
6. Place the 4 magnets into the recesses in each corner of of the midplates.
    * Magnetic field orientation doesn't matter, they connect to metal slugs
      in the Steam Machine chassis.
7. Place the board plate over the midplates, aligning the magnets and screw holes.
8. Fasten the boardplate with 8x screws in the screw holes along the edges.
9. Connect the e-ink panel to the breakout board.
    * Be careful when handling the flex that comes out of the panel, it is fragile
      and should not be bent or folded.
10. Place the battery in the recess, tucking its wire under the board plate and
    up into the clearance hole beside the feather.
11. Insert and fasten down the battery retainer/cover.
12. Plug the battery connecor into the feather.

Once you've assembled the unit it should be fairly solid and easily magnet to the
front of a Steam Machine!

You will need to build/flash the firmware onto the feather using the steps below.


## Usage

[Eventually we'll have an app up on Steam](https://store.steampowered.com/app/1222770)
but until then you can build an AppImage using the instructions further down in
this readme.

Once you have it built, take a look at `./docs/Inkterface Setup.pdf`, but the
basics are:
1. Enable bluetooth.
2. In desktop mode register as an app in Steam.
    * Or you can just run it directly.
3. Wait for it to discover your panel.
    * If it doesn't appear try clicking the reset button on the back and checking
      that bluetooth is enabled.
4. Select your panel, the name shown should match what's displayed on the inkterface.
    * They all start with an `INKTF-` prefix and then use a unique portion of the
      panels BLE MAC address.
5. On the configure screen, install the service using the button at the bottom
   right.
    * This will set itself up as a user service, so don't move the AppImage or
      that will break, no biggie though, you'll just need to re-run it.
    * Once the service is installed it might take 10-20 seconds for it to connect
      to the inkterface, that's fine, it just needs to go through discovery.
6. Finally you can click any of the readouts to adjust what they display.
    * We have several stats built in, but if you check out `include/panel-state.hpp`
      you can add any function that can return a `QString` or `double` and then
      `registerCollector()` to have it show up in the list.
      * There are some examples of stateful and idempotent collectors in `SysStats`,
        they get registered in the `PanelState` constructor.
7. Exit the configuration app.
    * The service is lightweight and runs in the background, you only need to
      run the config software to select a panel or to change what it displays.


## Building Interface

If you have Qt installed you should be able to open the cmake project in Qt Creator
and use that.

For better distributable builds you can setup a container for wider platform
support using the included `Containerfile`.

1. `podman build -t qt69-builder -f ./Containerfile`
2. `distrobox create --image qt69-builder --name qt69`
3. `distrobox enter qt69`
4. `./scripts/build.sh deploy`

That should produce an AppImage for you in the `./dist-linux-x86_64` folder.

This container uses an older version of Ubuntu and Qt 6.9 which should let us
build AppImages that will work on a wide range of modern systems.


## Building Firmware

For a mix of convenience and approachability this project uses
[PlatformIO](https://platformio.org), and in particular here you'll use the
[pio run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html)
command. Reading up on them can be useful, especial options like `--upload-port`
which could help if you have multiple serial ports.

If you're using the container from above you can just do:

1. `distrobox enter qt69`
2. `cd ./firmware`
3. If you have a **_V1_** feather: `pio run -e featherv1 -t upload`
4. If you have a **_V2_** feather: `pio run -e featherv2 -t upload`

And it should build and flash the firmware, if you want to get setup manually:

1. Install [PlatformIO](https://platformio.org) for your system.
2. `cd ./firmware`
3. If you have a **_V1_** feather: `pio run -e featherv1 -t upload`
4. If you have a **_V2_** feather: `pio run -e featherv2 -t upload`

Of course in both cases you need to have the ESP32 Feather attached via USB and
permission to interact with serial devices on your system.

**_NOTE:_** You can add build definitions and support for other boards by modifying
the `platformio.ini`, check it out to see the V1 and V2 feather environments.


## Design Docs

Planning out the UI and panel design is done in a Lunacy document you can find
in the `./design` folder.

Lunacy is an open source alternative to tools like Figma that can work entirely
offline.


## License

This project is licensed under the MIT License, see LICENSE.

This project depends on third-party software that is distributed under its own
licenses. Those licenses remain applicable to the respective components and
are not modified by this project's license.
