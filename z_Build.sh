#! /bin/bash
clear
# Package requirements (Arch Linux names):
# cmake, ninja, pkgconf, qt5-base, opencv, vtk, hdf5, openmpi, onnxruntime

# Set installation prefix path
otdir="$(dirname "$PWD")/opentrack-install"
# echo "Install prefix: $otdir"
otsrc="$PWD"
logfile="$otsrc/z_buildlog.txt"

arucodir="$otsrc/0_libaruco"

# Function: Pause
function pause(){
   echo "Press any key to continue"
   read -p "$*"
}

function clean_buildlog(){
    if [ -f "$logfile" ]; then
        rm "$logfile"
    fi
    echo "Previous log file deleted" 2>&1 | tee "$logfile"
}

function build_folder(){
    if [ ! -d "$otsrc/build" ]; then
        echo "Creating build folder"
        mkdir "$otsrc/build"
    fi
}

function clean_build(){
    if [ -d "$otsrc/build" ]; then
        echo "Deleting and recreating build folder"
        rm -rf "$otsrc/build"
        mkdir "$otsrc/build"
    fi
}

function link_xplane_sdk(){
    echo "Linking X-Plane SDK headers"
    cd "$otsrc/build"
    # Link X-Plane SDK headers
    mkdir -p xplane_sdk/CHeaders
    ln -sf /usr/include/xplane_sdk/Wrappers xplane_sdk/CHeaders/
    ln -sf /usr/include/xplane_sdk/Widgets xplane_sdk/CHeaders/
    ln -sf /usr/include/xplane_sdk/XPLM xplane_sdk/CHeaders/
}

function build_aruco(){
    if [ ! -d "$arucodir" ]; then
        mkdir "$arucodir"
        git clone https://github.com/opentrack/aruco.git "$arucodir"
    else
        cd "$arucodir"
        git pull
    fi
    if [ ! -d "$arucodir/build" ]; then
        mkdir "$arucodir/build"
    else
        rm -rf "$arucodir/build"
        mkdir "$arucodir/build"
    fi
    cd "$arucodir/build"
    cmake ".."
    cmake --build .
    make
    cd "$otsrc/build"
}

function build_opentrack(){
    echo "Building Opentrack"
    cd "$otsrc/build"
    if [[ $1 == "noaruco" ]]; then
        echo "Building without Aruco."
        cmake .. \
            -GNinja \
            -DCMAKE_BUILD_TYPE=Debug \
            -DSDK_XPLANE="$otsrc/xplane_sdk" \
            -DSDK_WINE=ON \
            -DCMAKE_INSTALL_PREFIX="$otdir"
            ## -DPATH_WINE_GPP="/home/bjoern/Downloads/wine-4.0-1-x86_64.pkg" \
            ninja
            # make -j$(nproc)
    fi
    if [[ $1 == "aruco" || $1 == "" ]]; then
        echo "Building with Aruco."
        build_aruco
        cmake .. \
            -GNinja \
            -DCMAKE_BUILD_TYPE=Debug \
            -DSDK_XPLANE="$otsrc/xplane_sdk" \
            -DSDK_ARUCO_LIBPATH="$arucodir/build/src/libaruco.a" \
            -DSDK_WINE=ON \
            -DCMAKE_INSTALL_PREFIX="$otdir"
            ninja
    fi

    if [ $? -ne '0' ]; then
        echo "ERROR: OPENTRACK BUILD FAILED"
        exit 1
    fi

    if [[ $1 == "install" || $2 == "install" ]]; then
        ninja install
        # make install
    fi
}

clean_buildlog
build_folder 2>&1 | tee "$logfile"
link_xplane_sdk 2>&1 | tee "$logfile"

clean_build
#build_opentrack "aruco" "install" 2>&1 | tee "$logfile"
build_opentrack "noaruco" "install" 2>&1 | tee "$logfile"

pause
