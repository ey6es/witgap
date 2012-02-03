set(HEADERS DatabaseThread.h)
set(SOURCES DatabaseThread.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(db ${SOURCES} ${HEADERS})
