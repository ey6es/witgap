set(SOURCES ClientApp.as)

find_program(MXMLC mxmlc)

add_custom_target(client.swf ALL
    ${MXMLC} ClientApp.as -output ${CMAKE_SOURCE_DIR}/web/client.swf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} SOURCES ${SOURCES})
