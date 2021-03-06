QT       += core gui
CONFIG   += debug_and_release

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ckb
TEMPLATE = app

macx {
	DESTDIR = $$PWD/../..
} else {
	DESTDIR = $$PWD/../../bin
}

QMAKE_MAC_SDK = macosx10.10
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
ICON = ckb-logo.icns
QMAKE_INFO_PLIST = ckb-info.plist
macx {
	LIBS += -framework Foundation -framework AudioToolbox
}

CKB_VERSION_STR = `cat $$PWD/../../VERSION`
DEFINES += CKB_VERSION_STR="\\\"$$CKB_VERSION_STR\\\""

SOURCES += main.cpp\
		mainwindow.cpp \
	kbwidget.cpp \
	colorbutton.cpp \
	settingswidget.cpp \
	kblightwidget.cpp \
	keymap.cpp \
	rgbwidget.cpp \
	media_linux.cpp \
	kblight.cpp \
	kbprofile.cpp \
	kbanimwidget.cpp \
	animscript.cpp \
	kbanim.cpp \
	animadddialog.cpp \
	keymap_us.cpp \
	keymap_gb.cpp \
	keymap_se.cpp \
	keymap_de.cpp \
	keymap_fr.cpp \
	animsettingdialog.cpp \
	gradientbutton.cpp \
	gradientdialog.cpp \
	gradientdialogwidget.cpp \
    kbmode.cpp \
    kb.cpp \
    rlistwidget.cpp \
    kbprofiledialog.cpp

HEADERS  += mainwindow.h \
	kbwidget.h \
	colorbutton.h \
	settingswidget.h \
	kblightwidget.h \
	keymap.h \
	rgbwidget.h \
	media.h \
	kblight.h \
	kbprofile.h \
	kbanimwidget.h \
	animscript.h \
	ckb-anim.h \
	kbanim.h \
	animadddialog.h \
	animsettingdialog.h \
	gradientbutton.h \
	gradientdialog.h \
	gradientdialogwidget.h \
    kbmode.h \
    kb.h \
    rlistwidget.h \
    kbprofiledialog.h

FORMS    += mainwindow.ui \
	kbwidget.ui \
	settingswidget.ui \
	kblightwidget.ui \
	kbanimwidget.ui \
	animadddialog.ui \
	animsettingdialog.ui \
	gradientdialog.ui \
    kbprofiledialog.ui

RESOURCES += \
	image.qrc \
    text.qrc

OBJECTIVE_SOURCES += \
	media_mac.m

DISTFILES += \
	ckb-info.plist
