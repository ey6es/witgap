set(HEADERS ConnectionManager.h Connection.h Session.h)
set(SOURCES ConnectionManager.cpp Connection.cpp Session.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-net ${SOURCES} ${HEADERS} ../../Protocol.h)
