set(HEADERS DatabaseThread.h)
set(SOURCES DatabaseThread.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-db ${SOURCES} ${HEADERS})
