set(SOURCES ClientApp.as)
set(OUTPUT ${CMAKE_SOURCE_DIR}/web/client.swf)

find_program(MXMLC mxmlc)

add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${MXMLC} ClientApp.as -output ${OUTPUT}
    DEPENDS ${SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(client.swf ALL DEPENDS ${OUTPUT})
