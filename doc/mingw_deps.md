# Building Dependencies in Mingw

On Ubuntu derivatives, you should be able to get all the required programs to cross compile from the package mingw-w64.

You should also setup a CMake toolchain file that looks something like this:
```
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_C_COMPILER /usr/bin/i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
SET(CMAKE_FIND_ROOT_PATH  /usr/i686-w64-mingw32)
SET(CMAKE_INSTALL_PREFIX /usr/i686-w64-mingw32)
```

## ICU

Clone https://github.com/unicode-org/icu

Create 2 build folders inside the folder you cloned, one for native compilation and the other to cross compile.

In the native build folder (let's call it buildA). Then inside that folder, run:

```
CXXFLAGS="-std=c++14" ../icu4c/source/runConfigureICU Linux/gcc
gmake
```

for 64 bit:
```
export MINGW_ARCH=x86_64-w64-mingw32
```
and 32 bit:
```
export MINGW_ARCH=i686-w64-mingw32
```

Then run:
```
CXXFLAGS="-std=c++14" ../icu4c/source/configure --build=x86_64-linux-gnu  --host=$MINGW_ARCH --with-cross-build=$ICU_GIT_PATH/buildA --prefix=/usr/$MINGW_ARCH/ --enable-static=yes --enable-shared=no
gmake
```

(assuming your mingw libraries/etc are in /usr/$MINGW_ARCH/)

This will probably fail because ICU names its static libraries wrong. You can fix it by creating symlinks in ${BUILD_FOLDER}/lib from libicu\*.a to libsicu\*.a for each file.
```
for x in libsicu*.a ; do ln -s $x ${x/libs/lib} ; done
```
Then in stubdata, do the same for libicudt.a > libsicudt.a.

Afterwards you can rerun gmake, then gmake install. 

At this point the libraries will be in your mingw prefix, but the names will still be wrong. 
You will need to create symlinks like during the build for each of these files in $PREFIX/lib:
- libsicutu.a
- libsicuin.a
- libsicuuc.a
- libsicutest.a
- libsicuio.a

Do not symlink libsicudt.a - this is a stub library and will cause errors if you try to link it.

You need to create a link from $PREFIX/lib/libicudt.a > $PREFIX/bin/sicudt.a instead. Or it might be $PREFIX/bin/libsicudt.a. After this you will be ready to compile Boost::locale.

## Boost locale

Boost thankfully is less finicky.

- Grab the Boost source from https://boostorg.jfrog.io/ui/native/main/release/ and unpack it somewhere.
- Run boostrap.sh.
- Creare a user-config.jam file with the following configuration:
    - using gcc : mingw32 : $MINGW_ARCH-g++ ;
- Run the following command:
```
./b2 --with-locale --user-config=user-config.jam -sICU_PATH=/usr/$MINGW_ARCH --prefix=/usr/$MINGW_ARCH variant=release address-model=64 toolset=gcc-mingw32 target-os=windows boost.locale.iconv=off
./b2 --with-locale --user-config=user-config.jam -sICU_PATH=/usr/$MINGW_ARCH --prefix=/usr/$MINGW_ARCH variant=release address-model=64 toolset=gcc-mingw32 target-os=windows boost.locale.iconv=off install
```
Note: Replace address model with 32 when building in 32-bit mode.

## yaml-cpp

yaml-cpp can be build as part of the normal build process. However, if you have yaml-cpp installed as a system library, it may be picked up by cmake even though it's not in the mingw prefix.

Use SABLE_REBUILD_YAML=ON to force building yaml-cpp manually.
