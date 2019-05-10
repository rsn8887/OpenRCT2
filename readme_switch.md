# OpenRCT2 (RollerCoaster Tycoon 2) port for Switch

This is my Switch port of OpenRCT2, a re-implementation of RollerCoaster Tycoon 2. A construction and management simulation video game that simulates amusement park management.

Data files from the original game are required to play this game. _RollerCoaster Tycoon 2_ data files are required to launch the game. Additionally, _RollerCoaster Tycoon 1_ data files are also supported.

Port by @rsn8887.

Visit my Patreon:
https://www.patreon.com/rsn8887

![](https://i.postimg.cc/TPH3VW8t/IMG-2411.jpg)
![](https://i.postimg.cc/50fz6zVj/Open-RCT2-cc.jpg)
![](https://i.postimg.cc/vmFfwj2r/Open-RCT2-bb.jpg)

# Thanks

Thanks to the many many developers who worked on this open source game.

Thanks to Chris Sawyer for writing the amazing original game.

Thanks to devkitPro for making Switch homebrew possible.

Thanks to Milenko for testing and making screenshots.

Thanks to my supporters on Patreon: Andyways, CountDuckula, Jason Crawford, Greg Gibson, Jesse Harlin, Özgür Karter, Matthew Machnee, Milenko, and RadicalR.

# Installation

- Download (see `assets` below), and unzip openrct2_switch.zip and copy the contents over to the `/switch/` folder on your sd card, so that you have a folder `/switch/openrct2` with many folders and files inside.

- Copy all files and folders from a working *Windows RollerCoaster Tycoon 2* installation to `/switch/openrct2/rct2/` so that you have a files and folders `/switch/openrct2/rct2/rct2.exe`, `/switch/openrct2/rct2/Data/`, `/switch/openrct2/rct2/Landscapes/`,  `/switch/openrct2/rct2/ObjData/` and many more files and folders there. For my testing, I used the files from the gog.com version.

- Optionally, you can also install the *Windows RollerCoaster Tycoon 1* files. Just copy your complete rct1 files to `/switch/openrct2/rct1/`. Then you can select the RCT1 scenarios, and in options you can select to show the rct1 title sequence. For my testing, I used the files from the gog.com version.

- Note: This game takes quite long to load at first, because it generates a bunch of cache files. Subsequent boots are much faster.

# Controls

 - Left analog stick = mouse pointer control 
 - Right analog stick = scroll the map 
 - R = left mouse click 
 - L = right mouse click 
 - ZR = hold to slow down analog joystick mouse, useful to precisely position the pointer 
 - ZL = hold to speed up analog joystick mouse 
 - A = right mouse click
 - B = left mouse click
 - Y = shift key, hold and move mouse up/down to build above the ground 
 - X = ctrl key, hold and move mouse to build multiple pieces at the same height above ground
 - Dpad up = zoom out (page up key) 
 - Dpad down = zoom in (page down key) 
 - Dpad left = rotate construction object (z key)
 - Dpad right = rotate camera (enter key) 
 - R3 (press right stick in) = open cheat menu (ctrl-alt-c)
 - Hold ZL + dpad left = close topmost window (backspace key)
 - Hold ZL + dpad right = exit construction mode (escape key)
 - Minus = toggle between three touch control modes: 
   * Touchpad style drag pointer with finger and tap to click, default: 
      * Move a single finger to move the mouse pointer. Use short tap for left click. Hold a single finger while tapping a second finger for right click. Drag with two fingers to drag and drop. 
   * Jump to finger without click 
      * The pointer jumps to the finger, but the finger doesn't click. Use L/R or A/B to click 
   * Jump to finger with tap click 
      * The pointer jumps to the finger, and a short tap also generates a left click 
 - Plus = bring up on-screen keyboard, useful for entering names, etc.
   * When pressing enter, it first erases existing text and replaces it with new text
 - Physical USB keyboard and mouse are supported. Not all mice work. There's a [mouse compatibility
chart](https://docs.google.com/spreadsheets/d/1Drbo5-QuSX901MwtOytSMuqRGxeIkq2HELM806I9dj0/edit#gid=0).

# Current Limitations

 - No network support

# Building

## Dependencies

- Switch compilation helper scripts
```
sudo -E dkp-pacman -S devkitpro-pkgbuild-helpers
```

- Switch libicu
```
git clone https://github.com/rsn8887/icu
cd icu
git checkout switch
cd ..
mkdir macos
mkdir switch
cd macos
../icu/icu4c/source/runConfigureICU MacOSX
make -j12
cd ../switch
source $DEVKITPRO/switchvars.sh
../icu/icu4c/source/configure --host aarch64-none-elf --prefix $DEVKITPRO/portlibs/switch/ --disable-shared --enable-static  --disable-samples --disable-tests --with-cross-build=$PWD/../macos
cp ../icu/icu4c/source/config/mh-linux ../icu/icu4c/source/config/mh-unknown
make -j12
sudo -E make install
```

- Switch libzip
```
git clone https://github.com/rsn8887/libzip
cd libzip
git checkout switch
cd ..
mkdir buildswitch
cd buildswitch
source $DEVKITPRO/switchvars.sh
cmake ../libzip \
-DENABLE_COMMONCRYPTO=OFF \
-DENABLE_GNUTLS=OFF \
-DENABLE_MBEDTLS=OFF \
-DENABLE_OPENSSL=OFF \
-DENABLE_WINDOWS_CRYPTO=OFF \
-DBUILD_TOOLS=OFF \
-DBUILD_REGRESS=OFF \
-DBUILD_EXAMPLES=OFF \
-DBUILD_DOC=OFF \
-DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/switch.cmake \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=$PORTLIBS_PREFIX \
-DCMAKE_INSTALL_LIBDIR=lib \
-DCMAKE_C_FLAGS="$CFLAGS $CPPFLAGS" \
-DCMAKE_CXX_FLAGS="CFLAGS -fno-exceptions -fno-rtti"
make -j12 install
```

- Switch libspeexdsp
```
git clone https://github.com/rsn8887/speexdsp
#might use libtoolize instead of glibtoolize if not on mac
glibtoolize
aclocal
autoconf
autoheader
automake --add-missing
source $DEVKITPRO/switchvars.sh
./configure --host aarch64-none-elf --prefix $DEVKITPRO/portlibs/switch/ --disable-shared --enable-static
sudo -E make install
```

## Main app

- Switch openRCT2
```
git clone https://github.com/rsn8887/openrct2
cd openRCT2
git checkout switch
cd ..
mkdir buildswitch
cd buildswitch
source $DEVKITPRO/switchvars.sh
cmake ../openrct2 \
-DSWITCH_BUILD=ON \
-DCMAKE_BUILD_TYPE=RELEASE
make -j12 openrct2_switch.zip
```

# Changelog
v1.04

- Fix right mouse click input
- Map object rotation to dpad left and camera rotation to dpad right
- Map backspace to zl+dpad left and escape to zl+dpad right to make room for new rotation mapping

v1.03

- Map shift, ctrl, backspace, and escape keys, useful to build above ground and quickly close windows.
- Map cheat menu hotkey, now works by pressing the right stick in.
- Make joystick pointer speed truly independent of fps. It now moves at constant speed even if the frame rate is low. 
- The filtering options now work as expected. In docked mode, `linear` gives a slightly blurry image, and `sharp nearest neighbor` gives perfectly sharp pixels. 
- General image quality improvements in both docked and handheld mode. The internal rendering now dynamically switches resolution between 1080p and 720p instead of always rendering at 720p. Also, the mouse pointer looks a bit sharper now in handheld mode. 
- The window scaling option now works and can be used to blow up or shrink down the whole game screen, including the user interface. 
- Different game resolutions should also work in principle now. This involves editing the window_width and window_height entries in `/switch/home/openrct2/config.ini`. Only 16x9 ratios will work correctly. I think the default choice of 960x540 looks the best. 

v1.02

- Fix mouse cursor not turning into a hand when hovering over hotspots in the park
- Reduce joystick pointer speed slightly for easier control in-game.

v1.01

- First release on Switch