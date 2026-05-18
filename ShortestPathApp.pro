QT       += core gui widgets

CONFIG   += c++17

TARGET    = ShortestPathApp
TEMPLATE  = app

SOURCES += main.cpp \
           main_window.cpp \
           graph_widget.cpp \
           graph.cpp \
           graph_validator.cpp \
           dp_solver.cpp \
           graph_io.cpp \
           report_writer.cpp

HEADERS += main_window.h \
           graph_widget.h \
           graph.h \
           graph_validator.h \
           dp_solver.h \
           graph_io.h \
           report_writer.h

# UTF-8 для MSVC под Windows.
win32-msvc*: QMAKE_CXXFLAGS += /utf-8
