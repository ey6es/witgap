set(HEADERS EditSceneDialog.h GoToSceneDialog.h Scene.h SceneManager.h SceneView.h)
set(SOURCES EditSceneDialog.cpp GoToSceneDialog.cpp Scene.cpp SceneManager.cpp SceneView.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-scene ${SOURCES} ${HEADERS})
