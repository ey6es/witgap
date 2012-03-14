find_package(Qt4 4.6.2 REQUIRED QtCore QtGui QtNetwork QtSql)
include(${QT_USE_FILE})

include_directories(.)
add_subdirectory(admin)
add_subdirectory(db)
add_subdirectory(net)
add_subdirectory(scene)
add_subdirectory(ui)

set(HEADERS LogonDialog.h CommandMenu.h ServerApp.h)
set(SOURCES LogonDialog.cpp CommandMenu.cpp ServerApp.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_executable(witgap-server ${SOURCES} ${HEADERS} main.cpp)
target_link_libraries(witgap-server ${QT_LIBRARIES}
    server-admin server-net server-db server-scene server-ui util)
