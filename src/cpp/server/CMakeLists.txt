find_package(Qt4 REQUIRED QtCore QtGui QtNetwork QtSql)
find_package(OpenSSL)
find_package(GeoIP)
include(${QT_USE_FILE})

include_directories(. ${OPENSSL_INCLUDE_DIR} ${GEOIP_INCLUDE_DIRS})
add_subdirectory(actor)
add_subdirectory(admin)
add_subdirectory(chat)
add_subdirectory(db)
add_subdirectory(http)
add_subdirectory(net)
add_subdirectory(peer)
add_subdirectory(scene)
add_subdirectory(script)
add_subdirectory(ui)

set(HEADERS CommandMenu.h LogonDialog.h MainWindow.h RuntimeConfig.h ServerApp.h SettingsDialog.h)
set(SOURCES CommandMenu.cpp LogonDialog.cpp MainWindow.cpp
    RuntimeConfig.cpp ServerApp.cpp SettingsDialog.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_executable(witgap-server ${SOURCES} ${HEADERS} main.cpp)
target_link_libraries(witgap-server server-admin server-net server-chat server-db server-http
    server-peer server-scene server-script server-actor server-ui util
    ${QT_LIBRARIES} ${OPENSSL_LIBRARIES} ${GEOIP_LIBRARIES})
