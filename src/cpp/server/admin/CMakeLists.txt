set(HEADERS AdminMenu.h)
set(SOURCES AdminMenu.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-admin ${SOURCES} ${HEADERS})
