# Inkterface, an e-ink faceplate for your Steam Machine.

## Required Hardware

* 1 x [Adafruit ESP32 Feather with 2MB PSRAM](https://www.adafruit.com/product/5400)
* 1 x [Adafruit eInk Breakout Friend](https://www.adafruit.com/product/4224)
* 1 x [Adafruit 5.83" Monochrome eInk Panel](https://www.adafruit.com/product/6397)
* 13 x M2.5 x 5mm Pan Head Machine Screws
    * [McMaster](https://www.mcmaster.com/92000A103/)
    * [Amazon](https://www.amazon.com/dp/B0GC58C2V5)
* 4 x [1/4" x 1/4" x 3/16" Stepped Magnet SB443-OUT](https://www.kjmagnetics.com/sb443-out-neodymium-stepped-block-magnet)


## Assembly

Check out the video `./docs/Inkterface Assembly.mp4` for a quick assembly tutorial!

**Coming Soon: a PDF guide!**

**Warning: The screws will thread themselves into the plastic, but be VERY gentle
as it is easy to strip the plastic away and need to re-print parts!**

1. Print parts from `./cad` folder.
    * Individual parts are included as separate STEP files, but a combined file
      with all parts that is ready for printing on a Stratasys F370 build plate
      is also provided as `Inkterface - Print Plate.step`.
2. Attach the ESP32 Feather and eInk Breakout to the Board Plate using 4x screws.
    * Having them held in place helps with soldering the connections and sizing your wires.
3. Solder wires between the pins of the two boards.
    * MOSI/MISO/SCK/GND all connect directly as expected.
    * 3V3 from the feather connects to VIN on the breakout.
        * The 3V3 pin on the breakout is a 3V3 output.
    * Then connect the following, like Feather -> Breakout:
        * PIN 6 -> ENA
        * PIN 9 -> BUSY
        * PIN 10 -> RST
        * PIN 11 -> SRCS
        * PIN 12 -> D/C
        * PIN 13 -> ECS
4. Place the e-ink panel into the matching recess in the faceplate part.
5. Fit the two midplates in.
6. Place the 4 magnets into the recesses in each corner of of the midplates.
7. Place the board plate over the midplates, aligning the magnets and screw holes.
8. Fasten the boardplate with 8x screws in the screw holes along the edges.
9. Connect the e-ink panel to the breakout board, being careful not to damage
   the cable!
10. Place the battery in the recess, tucking it's wire under the board plate and
    up into the clearance hole beside the feather.
11. Insert and fasten down the battery retainer/cover.
12. Plug the battery connecor into the feather.

Once you've assembled the unit it should be fairly solid and easily magnet to the
front of a Steam Machine!

You will need to build/flash the firmware onto the feather using the steps below.

## Running

If you have an AppImage build, just copy it to a SteamOS device and run it like:
* `./inkterface.AppImage`

That it will install and run itself as a user service. Make sure bluetooth is
enabled on the device and then if you bring a panel nearby it should connect.

You may need to hit the reset button on the back of the panel once you have it
next to the SteamOS host machine in case it's already connected somewhere else.


## Building Interface

If you have Qt installed you should be able to open the cmake project in Qt Creator
and that will should work for you!

For better distributable builds you can setup a container for wider platform
support using the included `Containerfile`.

1. `podman build -t qt69-builder -f ./Containerfile`
2. `distrobox create --image qt69-builder --name qt69`
3. `distrobox enter qt69`
4. `./scripts/build.sh deploy`

That should produce an AppImage for you in the `./dist-linux-x86_64` folder!

This container uses an older version of Ubuntu and Qt 6.9 which should let us
build AppImages that will work on a wide range of modern systems.


## Building Firmware

If you're using the container from above you can just do:

1. `distrobox enter qt69`
2. `cd ./firmware`
3. `pio run -t upload`

And it should build and flash the firmware, if you want to get setup manually:

1. Install [PlatformIO](https://platformio.org) for your system.
2. `cd ./firmware`
3. `pio run -t upload`

Of course in both cases you need to have the ESP32 Feather attached via USB and
permission to interact with serial devices on your system!

## Design Docs

Planning out the UI and panel design is done in a Lunacy document you can find
in the `./design` folder.

Lunacy is an open source alternative to tools like Figma that can work entirely
offline.
