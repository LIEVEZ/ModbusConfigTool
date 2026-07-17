QT += core gui widgets serialbus serialport

CONFIG += c++17 warn_on
TEMPLATE = app
TARGET = ModbusConfigTool

INCLUDEPATH += $$PWD/Source

SOURCES += $$files($$PWD/Source/*.cpp, true)
HEADERS += $$files($$PWD/Source/*.h, true)
RESOURCES += $$PWD/Source/Resources/Resources.qrc

DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
