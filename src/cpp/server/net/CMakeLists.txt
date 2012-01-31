set(HEADERS ConnectionManager.h Connection.h)
set(SOURCES ConnectionManager.cpp Connection.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(net ${SOURCES} ${HEADERS})
