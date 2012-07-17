set(HEADERS HttpConnection.h HttpManager.h)
set(SOURCES HttpConnection.cpp HttpManager.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-http ${SOURCES} ${HEADERS})
