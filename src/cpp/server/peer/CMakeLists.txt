set(HEADERS AbstractPeer.h Peer.h PeerConnection.h PeerManager.h)
set(SOURCES AbstractPeer.cpp Peer.cpp PeerConnection.cpp PeerManager.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-peer ${SOURCES} ${HEADERS} PeerProtocol.cpp PeerProtocol.h)
