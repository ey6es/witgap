set(HEADERS Actor.h Pawn.h)
set(SOURCES Actor.cpp Pawn.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-actor ${SOURCES} ${HEADERS})
