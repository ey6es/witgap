set(HEADERS AdminMenu.h EditUserDialog.h GenerateInvitesDialog.h RuntimeConfigDialog.h)
set(SOURCES AdminMenu.cpp EditUserDialog.cpp GenerateInvitesDialog.cpp RuntimeConfigDialog.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-admin ${SOURCES} ${HEADERS})
