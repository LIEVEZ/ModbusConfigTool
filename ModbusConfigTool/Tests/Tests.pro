QT += core gui widgets testlib serialbus serialport network

CONFIG += c++17 console testcase warn_on
TEMPLATE = app
TARGET = ModbusConfigToolTests

INCLUDEPATH += $$PWD/../Source

SOURCES += \
    $$PWD/test_main.cpp \
    $$PWD/Unit/test_project_model.cpp \
    $$PWD/Unit/test_register_value.cpp \
    $$PWD/Unit/test_strategy_engine.cpp \
    $$PWD/Unit/test_table_models.cpp \
    $$PWD/Unit/test_validation.cpp \
    $$PWD/Unit/test_value_converter.cpp \
    $$PWD/Unit/test_connection_service.cpp \
    $$PWD/Unit/test_connection_port_list_view.cpp \
    $$PWD/Unit/test_group_card_widget.cpp \
    $$PWD/Unit/test_group_canvas_view.cpp \
    $$PWD/Unit/test_group_register_config_dialog.cpp \
    $$PWD/Integration/test_project_repository.cpp \
    $$PWD/Integration/test_csv_gateway.cpp \
    $$PWD/Integration/test_main_window.cpp
SOURCES += $$PWD/Integration/test_modbus_runtime.cpp
SOURCES += $$files($$PWD/../Source/*.cpp, true)
SOURCES -= $$PWD/../Source/App/main.cpp
HEADERS += $$files($$PWD/../Source/*.h, true)
RESOURCES += $$PWD/../Source/Resources/Resources.qrc

DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
