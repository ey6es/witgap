set(HEADERS GoToSceneDialog.h Scene.h SceneManager.h ScenePropertiesDialog.h SceneView.h)
set(SOURCES GoToSceneDialog.cpp Scene.cpp SceneManager.cpp ScenePropertiesDialog.cpp SceneView.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-scene ${SOURCES} ${HEADERS})
