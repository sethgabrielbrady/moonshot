# AsteRisk (N64 Homebrew)

AsteRisk is a homebrew entry created for the N64Brew 2025 Game Jame and the Nintendo 64, developed using [libdragon](https://github.com/DragonMinded/libdragon) and [Tiny3D](https://github.com/HailToDodongo/tiny3d). Pilot your ship, mine asteroids, manage resources, and survive as long as possible!

## Features

- 3D gameplay with custom models
- Resource management (fuel, health, credits)
- Drone companion to assist with mining and repairs
- Multiple video modes depending on your hardware
- Multiple music tracks
- Rumble Pak support
- PAL and NTSC support

## Building

**Requirements:**
- [libdragon toolchain](https://github.com/DragonMinded/libdragon)
- [Tiny3D](https://github.com/HailToDodongo/tiny3d)
- GNU Make

**To build:**
```sh
make
```

The resulting ROM will be `asterisk.z64` (or your project name).

## Controls

- **Analog Stick:** Move ship
- **A Button:** Deflect asteroids
- **B Button:** Cancel/Back
- **C Buttons:** Command drone
  - **C-Left:** Send drone to mine
  - **C-Up:** Recall drone for repairs
  - **C-Down:** Send drone to station
- **R/Z:** Rotate camera
- **Start:** Pause/Menu

## Gameplay

Mine resources, dodge or deflect asteroids, conserve fuel, and command your drone to earn credits and stay alive in the asteroid belt. Bring resources to the loading station for payouts, repairs, and fuel. A full haul earns bonus credits!

## File Structure

- `src/` — Game source code
- `assets/` — Art, models, and audio
- `filesystem/` — Converted assets for ROM
- `build/` — Compiled ROM and intermediate files

## Credits

- **Programming:** brainpann
- **Libraries:** [libdragon](https://github.com/DragonMinded/libdragon), [Tiny3D](https://github.com/HailToDodongo/tiny3d)
- **Music:** DavidKBD — [Cosmic Journey Space Themed Music Pack](https://davidkbd.itch.io/cosmic-journey-space-themed-music-pack)
- **FONTS:** Dan Zadorozny [Space Ranger](https://www.iconian.com/), Din Studio [Striker](https://din-studio.com/)
- **MODELS/Sprites** brainpann
## License

This project is licensed under the [Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/).

Music by DavidKBD used under Creative Commons license.