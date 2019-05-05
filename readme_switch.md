Switch port is currently non-functional (WIP): it closes after black screen on startup. At least it compiles.

# Switch compilation instructions

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

## Installation instructions

- unzip openrct2.zip and copy to sd card folder "/switch/" so that you have "/switch/openrct2/openrct2.nro" and more files and folders.

- copy all data files and folders from a working Windows OpenRCT2 installation to "/switch/openrct2/rct2" so that you have a file "/switch/openrct2/rct2/rct2.exe" and many more files and folders