set(HEADERS ActorRepository.h DatabaseThread.h PeerRepository.h PropertyRepository.h
    SceneRepository.h SessionRepository.h UserRepository.h)
set(SOURCES ActorRepository.cpp DatabaseThread.cpp PeerRepository.cpp PropertyRepository.cpp
    SceneRepository.cpp SessionRepository.cpp UserRepository.cpp)
set(MTC_HEADERS SceneRepository.h SessionRepository.h UserRepository.h)

qt4_wrap_cpp(SOURCES ${HEADERS})
mtc_wrap_cpp(SOURCES ${MTC_HEADERS})
add_library(server-db ${SOURCES} ${HEADERS})
