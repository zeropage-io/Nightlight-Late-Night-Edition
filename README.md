# Nightlight-Late-Night-Edition
<a href="http://zeropage.io/"><img alt="Stormtrooper Nightlight" align="right" src="http://zeropage.io/wp-content/uploads/github-nightlight-small.gif" /></a>
An Arduino controlled night light with WS2812b RGB LEDs, touch sensor switches, a potpourri of nice lighting effects and a quite ugly piezo Star Wars(R) intro.

Watch this <a title="Stormtrooper Nightlight by zeropage" href="https://youtu.be/YXsqjSVHjwk" target="_blank">Video</a> on YouTube to see it in action!

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
