#!/bin/sh

qtwind="$(pwd)/winbuild/qt4.8"
qtx11d="$(pwd)/linbuild/qt4.8"

$qtx11d/bin/qmake -spec $qtx11d/mkspecs/linux-g++-64 "TARGET=start_linux" "INCLUDEPATH += ./linbuild/include" \
	"LIBS +=./linbuild/lib64/libgpgme.a ./linbuild/lib64/libgpg-error.a  -L./linbuild/lib64 -static-libgcc" \
	"DEFINES += STATICLINUX" "QMAKE_INCDIR_QT = $qtx11d/include" "QMAKE_LIBDIR_QT = $qtx11d/lib" \
	"QMAKE_QMAKE = $qtx11d/bin/qmake" "QMAKE_MOC = $qtx11d/bin/moc" "QMAKE_UIC = $qtx11d/bin/uic" \
	"QMAKE_RCC = $qtx11d/bin/rcc" "$@"

