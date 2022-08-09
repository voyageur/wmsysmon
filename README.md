# wmsysmon

wmsysmon is a program designed for use with the Window Maker window manager for the X Window System. It monitors the following system information: Memory usage, swap usage, I/O throughput, system uptime, hardware interrupts, paging and swap activity.

This is a continuation of code from www.gnugeneration.com (as indicated in original README)
## Dependencies
The dockapp needs the following X.Org libraries:
* libX11
* libXext
* libXpm

For common dockapp code, it also needs [libdockapp](https://www.dockapps.net/libdockapp)
## Compilation
Compilation requires the matching devel packages and a C compiler, then run make in src/ directory.

If you do not have a SMP system (quite rare these days), you can edit the Makefile to remove -DHI_INTS flag
