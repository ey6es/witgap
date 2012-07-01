set(HEADERS Scene.h SceneManager.h ScenePropertiesDialog.h SceneView.h Zone.h
    ZonePropertiesDialog.h)
set(SOURCES Scene.cpp SceneManager.cpp ScenePropertiesDialog.cpp SceneView.cpp Zone.cpp
    ZonePropertiesDialog.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-scene ${SOURCES} ${HEADERS})
