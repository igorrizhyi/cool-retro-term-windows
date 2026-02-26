TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += qmltermwidget
SUBDIRS += app

unix {
    desktop.files += cool-retro-term.desktop
    desktop.path += /usr/share/applications

    INSTALLS += desktop
}
