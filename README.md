# A fork of opentrack

This used to be a fork of Opentrack with extended Proton/WINE capabilities. Some of these have found their way into the upsteam repositories (from other contributors).    
Because of that and a general lack of interest in maintenance to keep it in sync with upstream, this fork has been rolled back to a minimum of changes.

The upstream Opentrack repository with the original Readme can be found [here](https://github.com/opentrack/opentrack).

&nbsp;

# 1. Modified X-Plane Plugin

A modified version of the X-Plane plugin with features geared to my personal requirements. See [its Readme](x-plane-plugin/README.md) for more information.
The standalone repository of the plugin is [here](https://github.com/JT8D-17/X-Plane-Opentrack-Plugin).    
To facilitate plugin compilation, this repostiory contains [the X-Plane SDK](https://developer.x-plane.com/sdk/plugin-sdk-downloads/).

&nbsp;

# 2. Build script

This fork supports a new build shell script named `z_Build.sh`. It was made for my Arch Linux system and requires the following packages: `cmake, ninja, pkgconf, opencv, vtk, hdf5, openmpi, onnxruntime`    
The default installation directory is `[parent directory of the opentrack source directory]/opentrack-install`, but edit it as you see fit.    
It will regenerate a log file named `z_buildlog.txt` upon every compilation.   
**My primary application for opentrack is x-plane, so the plugin for it will be built by default. If you do not need it, comment out all instances of the   
```-DSDK_XPLANE="$otsrc/xplane_sdk" \```   
and   
```link_xplane_sdk 2>&1 | tee "$logfile"```   
lines in the script.**   
Configure the script manually if you wish to build the script with Aruco support.    

&nbsp;

