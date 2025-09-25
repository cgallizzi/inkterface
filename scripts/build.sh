#!/bin/bash
set -e

if command -v nproc >/dev/null 2>&1; then
    CORES=$(nproc)
else
    CORES=$(sysctl -n hw.ncpu)
fi
PLATFORM=$(uname -s | tr [:upper:] [:lower:])
ARCH=$(uname -m | tr [:upper:] [:lower:])
BUILD_DIR=build-$PLATFORM-$ARCH
DIST_DIR=dist-$PLATFORM-$ARCH
BUILD_TYPE=MinSizeRel
DEPLOY=0
while test $# -gt 0; do
    case "$1" in
        debug)
            BUILD_TYPE=Debug
            ;;
        deploy)
            DEPLOY=1
            rm -rf $BUILD_DIR
            rm -rf $DIST_DIR
            rm -rf *.AppDir
            rm -rf *.AppImage
            ;;
        clean)
            rm -rf $BUILD_DIR
            rm -rf $DIST_DIR
            rm -rf *.AppDir
            rm -rf *.AppImage
            ;;
        *) echo "ignoring argument: $1"
    esac
    shift
done

echo "Building applications..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -B $BUILD_DIR -S .
cmake --build $BUILD_DIR --parallel $CORES

if [[ "$DEPLOY" == "1" && "$(uname)" == "Linux" ]]; then
    echo "Grabbing appimage builder..."
    wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
    chmod +x appimagetool-*.AppImage

    echo "Preparing appdir..."
    APP_DIR=mango-frunk.AppDir
    mkdir -p $APP_DIR/usr/bin
    mkdir -p $APP_DIR/usr/share/applications
    cp $BUILD_DIR/mango-frunk $APP_DIR/usr/bin/.
    cp resources/icon.png $APP_DIR/.
    cp resources/*.desktop $APP_DIR/usr/share/applications/.
    
    echo "Building appimage..."
    QTDIR=/usr/lib/qt6 ./appimagetool-*.AppImage deploy $APP_DIR/usr/share/applications/*.desktop
    VERSION=1.0 ./appimagetool-*.AppImage $APP_DIR

    # TODO: remove unecessary large files from appdir

    echo "Packaging release..."
    mkdir -p $DIST_DIR
    mv mango-frunk-*.AppImage $DIST_DIR/.
    pushd $DIST_DIR
    tar zcf ../mango-frunk-$PLATFORM-$ARCH-$(date +'%Y%m%d.%H%M%S').tar.gz *.AppImage
    popd
fi
