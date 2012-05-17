TEMPLATE = app
static:TARGET = obx_explorer.oar
else:TARGET = obx_explorer
QT += core \
    gui \
    webkit \
    sql \
    xml \
    dbus
HEADERS += association_dialog.h \
    mpdclient_dialog.h \
    book_scanner.h \
    metadata_reader.h \
    file_clipboard.h \
    database_utils.h \
    file_system_utils.h \
    tree_view.h \
    about_dialog.h \
    boox_action.h \
    obx_explorer.h \
    explorer_view.h
SOURCES += association_dialog.cpp \
    mpdclient_dialog.cpp \
    book_scanner.cpp \
    metadata_reader.cpp \
    file_clipboard.cpp \
    database_utils.cpp \
    file_system_utils.cpp \
    tree_view.cpp \
    about_dialog.cpp \
    boox_action.cpp \
    obx_explorer.cpp \
    main.cpp \
    explorer_view.cpp
FORMS += 
RESOURCES += obx_explorer.qrc
static:LIBS += /opt/onyx/arm/lib/static/libonyx_sys.a \
    /opt/onyx/arm/lib/static/libonyx_wireless.a \
    /opt/onyx/arm/lib/static/libonyx_wpa.a \
    /opt/onyx/arm/lib/static/libonyx_ui.a \
    /opt/onyx/arm/lib/static/libonyx_data.a \
    /opt/onyx/arm/lib/static/libonyx_cms.a \
    /opt/onyx/arm/lib/static/libonyx_screen.a \
    /opt/onyx/arm/lib/libpoppler-qt4.a \
    /opt/onyx/arm/lib/libpoppler.a \
    /opt/onyx/arm/lib/libfontconfig.a \
    -lquazip \
    -lQtMpdClient
else:LIBS += -lonyx_sys \
    -lonyx_wireless \
    -lonyx_wpa \
    -lonyx_ui \
    -lonyx_data \
    -lonyx_cms \
    -lonyx_screen \
    -lpoppler-qt4 \
    -lpoppler \
    -lfontconfig \
    -lquazip \
    -lQtMpdClient
