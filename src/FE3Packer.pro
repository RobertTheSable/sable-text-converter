TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    lib/asar/asardll.c \
    main.cpp \
    script.cpp

unix: SOURCES +=

HEADERS += \
    script.h \
    lib/asar/asardll.h \
    lib/utf8/utf8.h

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

LIBS +=  -ldl -lyaml-cpp -lstdc++fs -L$$PWD/lib/ -lasar
