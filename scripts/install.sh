#!/bin/sh
# Inkterface one-line installer for the Steam Machine (or any SteamOS/Linux box):
#   curl -fsSL https://raw.githubusercontent.com/OWNER/inkterface/main/scripts/install.sh | sh
# Downloads the latest release AppImage to ~/Applications and launches it so
# you can pick your panel and install the background service.
set -e

REPO="OWNER/inkterface"
APP_DIR="$HOME/Applications"
APP_PATH="$APP_DIR/Inkterface.AppImage"

echo "Looking up the latest Inkterface release..."
URL=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" |
    grep '"browser_download_url"' | grep 'Inkterface.AppImage' | head -n 1 | cut -d '"' -f 4)
if [ -z "$URL" ]; then
    echo "Could not find an AppImage in the latest release of $REPO" >&2
    exit 1
fi

echo "Downloading $URL"
mkdir -p "$APP_DIR"
curl -fL -o "$APP_PATH" "$URL"
chmod +x "$APP_PATH"

echo ""
echo "Installed to $APP_PATH"
echo "Launching it now - pick your INKTF panel, then use the Install button"
echo "(bottom right) to set up the background service."
echo "Tip: keep the AppImage where it is; the service points at this location."
exec "$APP_PATH"
