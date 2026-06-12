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
GITREV="g$(git describe --always --dirty | tr [:lower:] [:upper:])"
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

if [ "$PLATFORM" == "darwin" ]; then
    CMAKE=qt-cmake
else
    CMAKE=cmake
fi

echo "Building applications..."
$CMAKE -DGITREV=$GITREV -DCMAKE_BUILD_TYPE=$BUILD_TYPE -B $BUILD_DIR -S .
cmake --build $BUILD_DIR --parallel $CORES

if [[ "$DEPLOY" == "1" && "$(uname)" == "Linux" ]]; then
    QT_DIR=$(dirname $(qmake6 -v | tail -1 | cut -d' ' -f6))

    echo "Grabbing appimage builder..."
    wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
    chmod +x appimagetool-*.AppImage

    echo "Preparing appdir..."
    APP_DIR=inkterface.AppDir
    mkdir -p $APP_DIR/usr/bin
    mkdir -p $APP_DIR/usr/share/applications
    mkdir -p $APP_DIR/usr/lib/qt6/plugins
    cp $BUILD_DIR/inkterface $APP_DIR/usr/bin/.
    cp resources/icon.png $APP_DIR/.
    cp resources/*.desktop $APP_DIR/usr/share/applications/.
    cp -r $QT_DIR/plugins/tls $APP_DIR/usr/lib/qt6/plugins/.

    echo "Building appimage..."
    APPIMAGE_EXTRACT_AND_RUN=1 QTDIR=$QT_DIR ./appimagetool-*.AppImage deploy $APP_DIR/usr/share/applications/*.desktop
    APPIMAGE_EXTRACT_AND_RUN=1 VERSION=$GITREV ./appimagetool-*.AppImage $APP_DIR

    # TODO: remove unecessary large files from appdir

    echo "Packaging release..."
    mkdir -p $DIST_DIR
    mv inkterface-*.AppImage $DIST_DIR/.
    # don't create archive for wip
    if [[ "$GITREV" != *"DIRTY"* ]]; then
        pushd $DIST_DIR
        tar zcf ../inkterface-$PLATFORM-$ARCH-$GITREV.tar.gz *.AppImage
        popd
    fi
    cp resources/launch.sh $DIST_DIR/.
    cp $DIST_DIR/inkterface-*.AppImage $DIST_DIR/inkterface.AppImage
fi
