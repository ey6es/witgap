find_package(Qt4 4.6.2 REQUIRED QtCore QtGui QtNetwork QtSql)
find_package(OpenSSL)
include(${QT_USE_FILE})

include_directories(. ${OPENSSL_INCLUDE_DIR})
add_subdirectory(actor)
add_subdirectory(admin)
add_subdirectory(db)
add_subdirectory(net)
add_subdirectory(scene)
add_subdirectory(ui)

set(HEADERS CommandMenu.h LogonDialog.h MainWindow.h ServerApp.h SettingsDialog.h)
set(SOURCES CommandMenu.cpp LogonDialog.cpp MainWindow.cpp ServerApp.cpp SettingsDialog.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_executable(witgap-server ${SOURCES} ${HEADERS} main.cpp)
target_link_libraries(witgap-server ${QT_LIBRARIES} ${OPENSSL_LIBRARIES}
    server-admin server-net server-db server-scene server-actor server-ui util)
