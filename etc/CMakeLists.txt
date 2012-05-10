find_package(Qt4 4.6.2)

qt4_create_translation(en.qm ${CMAKE_SOURCE_DIR}/src en.ts)

add_custom_target(translations ALL ${CMAKE_COMMAND} -E copy en.qm ${CMAKE_SOURCE_DIR}/etc
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/en.qm)
