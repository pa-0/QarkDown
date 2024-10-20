QT += network widgets

win32 {
    RC_FILE += cfg/win.rc
    RESOURCES += compilers-windows.qrc
}

mac {
    OSX_FILES.files = gfx/icon.icns gfx/document.icns
    OSX_FILES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += OSX_FILES
    QMAKE_INFO_PLIST = cfg/Info.plist
    RESOURCES += compilers-osx.qrc
    CONFIG += cocoa
    LIBS += -framework Cocoa
}

mac:cocoa {
    OBJECTIVE_SOURCES += \
        mac/main.mm \
        mac/mainwindow.mm \
        mac/cocoaappdelegate.mm
    HEADERS += \
        mac/cocoaappdelegate.h
} else {
    SOURCES += \
        main.cpp \
        mainwindow.cpp
}

linux-g++ {
    RESOURCES += cfg/linux.qrc
    RESOURCES += compilers-linux.qrc
}

RESOURCES += \
    styles.qrc \
    misc.qrc

CONFIG(release, debug|release) {
    DEFINES += BUILD_RELEASE
}
CONFIG(debug, debug|release) {
    DEFINES += BUILD_DEBUG
}


HEADERS += \
    defines.h\
    qarkdownapplication.h \
    mainwindow.h \
    preferencesdialog.h \
    peg-markdown-highlight/pmh_parser.h \
    peg-markdown-highlight/pmh_definitions.h \
    peg-markdown-highlight/highlighter.h \
    editor/qarkdowntextedit.h \
    editor/linenumberingplaintextedit.h \
    peg-markdown-highlight/pmh_styleparser.h \
    markdowncompiler.h \
    logger.h \
    filesearchdialog.h
SOURCES += \
    qarkdownapplication.cpp \
    preferencesdialog.cpp \
    peg-markdown-highlight/pmh_parser.c\
    peg-markdown-highlight/highlighter.cpp \
    editor/qarkdowntextedit.cpp \
    editor/linenumberingplaintextedit.cpp \
    peg-markdown-highlight/pmh_styleparser.c \
    markdowncompiler.cpp \
    logger.cpp \
    filesearchdialog.cpp

FORMS += \
    preferencesdialog.ui \
    filesearchdialog.ui

OTHER_FILES += \
    cfg/win.rc \
    cfg/Info.plist \
    cfg/linux.qrc \
    template.html

DISTFILES += \
    template_kburke.html












