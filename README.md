# Opentrack with extended WINE/Proton handling capabilities

This is an experimental fork of opentrack. For informations about the main project, please refer to the original [opentrack Github repository](http://github.com/opentrack/opentrack).

&nbsp;

## New/Changed features

### 1. Custom WINE runtime paths

![Screenshot_20210502_005227](https://user-images.githubusercontent.com/36554499/116796932-cf2a8a80-aae0-11eb-8a72-a22793f690f7.jpg)

If users have WINE apps requiring a certain WINE release, but do not wish to keep the system's WINE release out of date, opentrack now accepts a custom path to a WINE runtime.    
It should be pointed to a folder containing `/bin/wine`. The default value is `/usr`, which is the standard value on an Arch Linux system.
*Proton releases may not work properly yet.*    
The WINE runtime will use the prefix specified in the field above.

&nbsp;

### 2. Automatic detection of Steam apps using Proton

![Screenshot_20210502_004747](https://user-images.githubusercontent.com/36554499/116797050-b373b400-aae1-11eb-8023-c1d12fd71c02.jpg)

The Proton app selection method and user interface has been completely reworked.   
App selection methodology and notes:
- Scans the default and any additional Steam libraries specified in `libraryfolders.vdf` for `appmanifest_*` files.
- Pulls the appid and name from all manifest files.
- Looks for `steamapps/compatdata/[appid]` in all Steam libraries.
- When found, extracts Proton version information from `version` and/or `config_info`.    
- Looks for the Proton runtime specified by that information (taking `/steam/root/compatibilitytools.d` into consideration).   
*Note: Switching an app from Proton > 5.9 back to a version < 5.9 may cause this logic to break. In this case, delete the config_info file from `steamapps/compatdata/[appid]`. This is totally safe.*
- When a Steam app's Proton runtime folder and prefix folder was found, it will be added to the list of available apps.
- The selected app is stored in the selected opentrack profile when pressing "OK" button.
- Proton paths are not stored in the opentrack profile for the game; only the appid is.    
When tracking starts in Opentrack, most of the above logic runs again and passes the Proton runtime path and prefix path (plus various WINE environment variables) to opentrack's WINE wrapper.    
This avoids users having to relaunch the profile editor when changing a game's Proton runtime or moving Steam libraries around.

**See [below](#versioninfo) for Proton version compatibility information.**

&nbsp;

### 3. Wine wrapper selection

![Screenshot_20210502_002706](https://user-images.githubusercontent.com/36554499/116797293-8d034800-aae4-11eb-9c77-2e3c1f9fb496.jpg)

Opentrack now supports handling multiple WINE wrappers stored in `/libexec/opentrack`.   
These wrappers must be named so that the file filter, looking for `opentrack-wrapper-wine*.exe.so`, can pick them up.

**See [below](#versioninfo) for WINE version compatibility information.**

&nbsp;

### 4. Build script

This fork supports a new build shell script named `z_Build.sh`. It was made for my Arch Linux system and requires the following packages: `cmake, ninja, pkgconf, qt5-base, opencv, vtk, hdf5, openmpi, xplane-sdk-devel onnxruntime`    
The Default installation directory is `[parent directory of the opentrack source directory]/opentrack-install`, but edit it as you see fit.    
It will regenerate a log file named `z_buildlog.txt` upon every compilation.   
**My primary aqpplication for opentrack is x-plane, so the plugin for it will be built by default. If you do not need it, comment out all instances of the   
```-DSDK_XPLANE="$otsrc/xplane_sdk" \```   
and   
```link_xplane_sdk 2>&1 | tee "$logfile"```   
lines in the script.**   
Supported parameters:    
- `aruco` or none: Build with Aruco support    
- `aruco clean`: Build with Aruco support; delete old `build` folder before compilation    
- `noaruco`: Build without Aruco support    
- `noaruco clean`: Build without Aruco support; delete old `build` folder before compilation    

&nbsp;

### 5. Modified X-Plane Plugin

This repository includes a modified version of the X-Plane plugin. See [its Readme](x-plane-plugin/README.md) for more information.
The standalone repository of the plugin is [here](https://github.com/JT8D-17/X-Plane-Opentrack-Plugin).

&nbsp;

<a name="versioninfo"></a>
## WINE/Proton version information

A few findings I've collected in the course of implementing the above:
- Opentrack's WINE wrapper, as any binary compiled with wineg++/winegcc, is only forward compatible, which means that a wrapper compiled against WINE 4.0 will work in WINE 6.5, but not the other way around.    
As this may break a opentrack compatibility with games using a WINE/Proton release older than the one the wrapper was compiled against and to save users a bit of a headache compiling a wrapper on their own, this fork provides `opentrack-wrapper-wine-4.0-fallback.exe.so`.    
It was compiled against WINE 4.0, is being installed to `/libexec/opentrack` by default and can be selected from the wrapper selector in the WINE Settings dialog.
- In order to run Proton releases newer than 5.9 (i.e. 5.13 and newer) which are being launched inside a container (using bubblewrap and pressure-vessel from Steam Runtime Soldier), Opentrack's WINE wrapper **must** be pointed to the game prefix and the **exact** Proton runtime folder used by the game.
&nbsp;

## Throwaway features

- There's a `PATH_WINE_GPP` cmake flag now which sets a custom path to `/usr/bin/wineg++` and the `-B` flag for `wineg++`. It defaults to the system's `wineg++` when not set and was intended to provide a method to compile a WINE wrapper against a non-system WINE installation. However, I simply could not get the wrapper to compile when setting it, but I left it in in case somebody wants to experiment with it. See `/proto-wine/CMakeLists.txt` for the implementation.

&nbsp;

## License and warranty

*Fork author's note: This is from opentrack's original readme*

Almost all code is licensed under the [ISC license](https://en.wikipedia.org/wiki/ISC_license). There are very few proprietary dependencies. There is no copyleft code. See individual files for licensing and authorship information.

See [WARRANTY.txt](WARRANTY.txt) for applying warranty terms (that is, disclaiming possible pre-existing warranty) that are in force unless the software author specifies their own warranty terms. In short, we disclaim all possible warranty and aren't responsible for any possible damage or losses.

The code is held to a high-quality standard and written with utmost care; consider this a promise without legal value. Despite doing the best we can not to injure users' equipment, software developers don't want to be dragged to courts for imagined or real issues. Disclaiming warranty is a standard practice in the field, even for expensive software like operating systems.
