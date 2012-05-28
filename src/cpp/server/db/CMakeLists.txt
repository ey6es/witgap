set(HEADERS DatabaseThread.h PeerRepository.h PropertyRepository.h
    SceneRepository.h SessionRepository.h UserRepository.h)
set(SOURCES DatabaseThread.cpp PeerRepository.cpp PropertyRepository.cpp
    SceneRepository.cpp SessionRepository.cpp UserRepository.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-db ${SOURCES} ${HEADERS})
