set(HEADERS Component.h Window.h)
set(SOURCES Component.cpp Window.cpp)

add_library(server-ui ${SOURCES} ${HEADERS})
