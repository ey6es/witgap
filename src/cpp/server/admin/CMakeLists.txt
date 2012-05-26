set(HEADERS AdminMenu.h EditUserDialog.h RuntimeConfigDialog.h)
set(SOURCES AdminMenu.cpp EditUserDialog.cpp RuntimeConfigDialog.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-admin ${SOURCES} ${HEADERS})
