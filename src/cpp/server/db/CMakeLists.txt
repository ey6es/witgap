set(HEADERS DatabaseThread.h SessionRepository.h UserRepository.h)
set(SOURCES DatabaseThread.cpp SessionRepository.cpp UserRepository.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-db ${SOURCES} ${HEADERS})
