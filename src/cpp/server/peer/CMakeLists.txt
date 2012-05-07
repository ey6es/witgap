set(HEADERS PeerConnection.h PeerManager.h)
set(SOURCES PeerConnection.cpp PeerManager.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-peer ${SOURCES} ${HEADERS})
