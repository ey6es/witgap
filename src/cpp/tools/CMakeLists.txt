find_package(Qt4 REQUIRED QtCore)
include(${QT_USE_FILE})

add_executable(mtc mtc.cpp)
target_link_libraries(mtc ${QT_LIBRARIES})
