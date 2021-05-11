#!/bin/sh

qtwind="$(pwd)/winbuild/qt4.5"
qtx11d="$(pwd)/linbuild/qt4.5"
#mingwb="$(ls /usr/ | grep mingw32 | head --lines 1)"
mingwb=i586-mingw32msvc
	
$qtx11d/bin/qmake -spec  $qtx11d/mkspecs/win32-g++ "TARGET=start_windows" "INCLUDEPATH += ./winbuild/include" "LIBS +=./winbuild/lib/libgpgme.a ./winbuild/lib/libgpg-error.a" "DEFINES += STATICWINDOWS" "QMAKE_CC = $mingwb-gcc -m32" "QMAKE_CXX = $mingwb-g++ -m32" "QMAKE_INCDIR_QT = $qtwind/include" "QMAKE_LIBDIR_QT = $qtwind/lib" "QMAKE_LINK = $mingwb-g++" "QMAKE_COPY_DIR = cp -r" "QMAKE_COPY = cp" "QMAKE_COPY_DIR = cp -r" "QMAKE_MOVE = mv" "QMAKE_DEL_FILE = rm" "QMAKE_CHK_DIR_EXISTS = test -d" "QMAKE_QMAKE = $qtx11d/bin/qmake" "QMAKE_MOC = $qtx11d/bin/moc" "QMAKE_UIC = $qtx11d/bin/uic" "QMAKE_RCC = $qtx11d/bin/rcc" "QMAKE_RC = $mingwb-windres" "QMAKE_LFLAGS += -Wl,-subsystem,windows" "$@"

