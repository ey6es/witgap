set(HEADERS ChatWindow.h)
set(SOURCES ChatWindow.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-chat ${SOURCES} ${HEADERS} ChatCommands.cpp ChatCommands.h)
