find_package(Qt4 4.6.2 REQUIRED QtCore QtNetwork)
include(${QT_USE_FILE})

set(HEADERS Callback.h)
set(SOURCES Callback.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(util ${SOURCES} ${HEADERS} General.h General.cpp Mailer.h Mailer.cpp)
