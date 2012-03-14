set(HEADERS SceneView.h)
set(SOURCES SceneView.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-scene ${SOURCES} ${HEADERS})
