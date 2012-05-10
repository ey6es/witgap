set(HEADERS AbstractPeer.h Peer.h PeerConnection.h PeerManager.h)
set(SOURCES AbstractPeer.cpp Peer.cpp PeerConnection.cpp PeerManager.cpp PeerProtocol.cpp)
set(MTC_HEADERS PeerManager.h PeerProtocol.h)

qt4_wrap_cpp(SOURCES ${HEADERS})
mtc_wrap_cpp(SOURCES ${MTC_HEADERS})
add_library(server-peer ${SOURCES} ${HEADERS} ${MTC_HEADERS})
