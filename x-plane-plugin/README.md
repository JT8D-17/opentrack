# Opentrack Plugin for X-Plane

Based on commit [b8e7865a017363e9d3876539936a6f73310a99d9](https://github.com/opentrack/opentrack/commit/b8e7865a017363e9d3876539936a6f73310a99d9)  (2019/12/24) from Opentrack's repository.

&nbsp;

## Changes

- Plugin output is not enabled after plugin start
- Added On/Off commands for better plugin control (see below)
- Implemented a plugin-specific, writable dataref (see below)
- Robustness improvements (e.g. command phase detection)
- Beautification for plugin strings and commands
- Added some optional debug output strings for X-Plane's dev console and Log.txt (debug_strings in line 64; set to "1" and recompile)
- Added this readme
- Added X-Camera output support (12/09/2020)

&nbsp;

## Installation

Move `opentrack.xpl` from the `Opentrack/libexec/opentrack` folder into `X-Plane 11/Resources/plugins`.

&nbsp;

## Utilization

The plugin can be controlled via command (and keyboard) and/or dataref.

&nbsp;
#### Commands

These can also be set from X-Plane's keyboard assignment menu.

	Opentrack/Toggle: Toggles plugin output to X-Plane
	Opentrack/On: Enables plugin output to X-Plane  
	Opentrack/Off: Disables plugin output to X-Plane  
	Opentrack/Toggle_Translation: Toggles plugin translation information output to X-Plane

&nbsp;
#### Datarefs

For checking the plugins's status, there is the following dataref:
  
	Opentrack/Tracking: Writable; `0/1` = Tracking disabled/enabled


&nbsp;
#### With X-Camera
When the presence of the [X-Camera](https://www.stickandrudderstudios.com/x-camera/) plugin is detected at startup, head position output will automatically be piped to its datarefs (see X-Camera manual).

- Make sure that "TrackIR" is enabled (checked) in X-Camera's control panel. The checkbox also controls headtracking output to X-Camera when Opentrack and this plugin's output is active.
- This plugin's commands (see above) are still active and required to control output.
    
&nbsp;

## Known issues

- MacOS untested
- Integration with other X-Plane camera plugins may not work as desired 

&nbsp;

## License

See also the header of _plugin.c_.

_Copyright (c) 2013, Stanislaw Halik <sthalik@misaki.pl>   
 Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies._
