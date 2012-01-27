find_package(Qt4 4.6.2 REQUIRED QtCore)
include(${QT_USE_FILE})

add_executable(witgapd main.cpp)
target_link_libraries(witgapd ${QT_LIBRARIES})
