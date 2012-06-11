set(HEADERS ConnectionManager.h Connection.h Session.h)
set(SOURCES ConnectionManager.cpp Connection.cpp Session.cpp)
set(MTC_HEADERS Session.h)

qt4_wrap_cpp(SOURCES ${HEADERS})
mtc_wrap_cpp(SOURCES ${MTC_HEADERS})
add_library(server-net ${SOURCES} ${HEADERS} ${MTC_HEADERS} ../../Protocol.h)
