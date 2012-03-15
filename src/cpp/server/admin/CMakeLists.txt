set(HEADERS AdminMenu.h EditUserDialog.h)
set(SOURCES AdminMenu.cpp EditUserDialog.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-admin ${SOURCES} ${HEADERS})
