find_package(Qt4 4.6.2 REQUIRED QtCore QtNetwork)
include(${QT_USE_FILE})

set(HEADERS Callback.h General.h)
set(SOURCES Callback.cpp General.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(util ${SOURCES} ${HEADERS} Mailer.h Mailer.cpp)
