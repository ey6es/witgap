set(HEADERS Component.h Window.h)
set(SOURCES Component.cpp Window.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-ui ${SOURCES} ${HEADERS} Layout.cpp Layout.h Border.cpp Border.h)
