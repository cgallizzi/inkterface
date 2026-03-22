FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    wget \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    python3-venv \
    pkg-config \
    sudo \
    ca-certificates \
    file \
    patchelf \
    desktop-file-utils \
    libgl1-mesa-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libxcb1-dev \
    libheif1 \
    libheif-dev \
    libjxr0 \
    libjxr-dev \
    libfuse2 \
    libxcb-render0-dev \
    libxcb-shape0-dev \
    libxcb-xfixes0-dev \
    libxcb-randr0-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-icccm4-dev \
    libxcb-sync-dev \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libxcb-util-dev \
    libxcb-cursor-dev \
    libdbus-1-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libglu1-mesa-dev \
    libssl-dev \
    libpulse-dev \
    libasound2-dev \
    libudev-dev \
    unzip \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# aqtinstall to install Qt, platformio to build firmware
RUN pip3 install --no-cache-dir aqtinstall platformio

# setup some user stuff that is helpful in distrobox
ARG USERNAME=developer
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID $USERNAME \
 && useradd -m -u $UID -g $GID -s /bin/bash $USERNAME \
 && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER $USERNAME
WORKDIR /home/$USERNAME

ENV QT_ROOT=/opt/qt
RUN mkdir -p $QT_ROOT && chown -R $USERNAME:$USERNAME $QT_ROOT
RUN aqt install-qt \
    linux \
    desktop \
    6.9.0 \
    linux_gcc_64 \
    --outputdir $QT_ROOT \
    --modules qtserialport qtconnectivity
# available modules:
# qt3d, qt5compat, qtcharts, qtdatavis3d, qtgraphs, qtgrpc, qthttpserver,
# qtimageformats, qtlanguageserver, qtlocation, qtlottie qtmultimedia,
# qtnetworkauth, qtpdf, qtpositioning, qtquick3d, qtquick3dphysics,
# qtquickeffectmaker, qtquicktimeline, qtremoteobjects, qtscxml, qtsensors,
# qtserialbus, qtshadertools, qtspeech, qtvirtualkeyboard, qtwaylandcompositor,
# qtwebchannel, qtwebengine, qtwebsockets, qtwebview
ENV PATH=$QT_ROOT/6.9.0/gcc_64/bin:$PATH
ENV CMAKE_PREFIX_PATH=$QT_ROOT/6.9.0/gcc_64

# virtual fs mounting doesn't always work well
ENV APPIMAGE_EXTRACT_AND_RUN=1

WORKDIR /workspace
CMD ["/bin/bash"]
