set(HEADERS ScriptContext.h)
set(SOURCES ScriptContext.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-script ${SOURCES} ${HEADERS})
