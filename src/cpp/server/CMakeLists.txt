find_package(Qt4 4.6.2 REQUIRED QtCore QtGui QtNetwork QtSql)
include(${QT_USE_FILE})

include_directories(.)
add_subdirectory(net)
add_subdirectory(db)
add_subdirectory(ui)

set(HEADERS ServerApp.h)
set(SOURCES main.cpp ServerApp.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_executable(witgap-server ${SOURCES} ${HEADERS})
target_link_libraries(witgap-server ${QT_LIBRARIES} server-net server-db server-ui util)
