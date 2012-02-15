find_package(Qt4 4.6.2 REQUIRED QtCore)
include(${QT_USE_FILE})

set(HEADERS Callback.h)
set(SOURCES Callback.cpp)

add_library(util ${SOURCES} ${HEADERS})
