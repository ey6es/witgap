set(SOURCES ClientApp.as)
set(OUTPUT ${CMAKE_SOURCE_DIR}/www/client.swf)

find_program(MXMLC mxmlc ${CMAKE_SOURCE_DIR}/../flex/bin)

add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${MXMLC} ClientApp.as -static-link-runtime-shared-libraries -output ${OUTPUT}
    DEPENDS ${SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(client.swf ALL DEPENDS ${OUTPUT})
