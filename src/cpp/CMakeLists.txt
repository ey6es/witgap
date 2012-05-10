include_directories(.)

function(mtc_wrap_cpp SOURCEFILES)
    foreach(HEADER ${ARGN})
        get_filename_component(BASENAME ${HEADER} NAME_WE)
        get_filename_component(BASEPATH ${HEADER} PATH)
        set(OUTPUT ${BASEPATH}mtc_${BASENAME}.cxx)

        add_custom_command(OUTPUT ${OUTPUT}
            COMMAND ${EXECUTABLE_OUTPUT_PATH}/mtc
            ARGS -o ${OUTPUT} ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}
            DEPENDS ${EXECUTABLE_OUTPUT_PATH}/mtc ${HEADER})

        set(${SOURCEFILES} ${${SOURCEFILES}};${OUTPUT} PARENT_SCOPE)
    endforeach(HEADER)
endfunction(mtc_wrap_cpp)

add_subdirectory(server)
add_subdirectory(tools)
add_subdirectory(util)
