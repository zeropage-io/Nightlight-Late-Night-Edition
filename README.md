# Nightlight-Late-Night-Edition
An Arduino controlled night light with WS2812b RGB LEDs, touch sensor switches, a potpourri of nice lighting effects and a quite ugly piezo Star Wars(R) intro.

Get the STL-files for 3D printing <a title="Nightlight Late-Night Edition on Thingiverse" href="http://www.thingiverse.com/thing:1784830" target="_blank">on Thingiverse</a>.<br />
Full build instructions here <a title="Build instructions for Nightlight Late-Night Edition on zeropage.io" href="http://zeropage.io/nightlight-late-night-edition/" target="_blank">Nightlight Late-Night Edition</a>.

Ein Nachtlicht mit unterschiedlichen Designs. Arduino-gesteuerte RGB LED Lichteffekte, Berührungssensoren und Imperial March Startmusik, die aus einem piepsigen Piezo-Summer tönt (abschaltbar;-).

### History
1.3, 12.11.2016
- Added new mode eWHITE that switches all LEDs to, yes, white.

1.2, 13.09.2016
- eSTROBE: Using EVERY_N_MILLISECONDS macro did not work. Rolled my own.
- Very long touching mode button did not work. Used wrong variable names. Fixed.
- eBREATH mode looks ugly on lower brightness settings. Changed amplitude.

1.1, 12.09.2016
- Usability: increase sensor 2nd function waiting time. one second is too short and might induce unwanted triggers.
- Usability: add 4 sec long touch (3rd function) for mode sensor to quickly jump to first mode.
- Removed unnecessary code from eBREATHE mode.

1.0, 11.09.2016
- Initial release.
