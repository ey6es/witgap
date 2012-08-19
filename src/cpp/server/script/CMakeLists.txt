set(HEADERS )
set(SOURCES )

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-script ${SOURCES} ${HEADERS}
    Lexer.cpp Parser.cpp Script.cpp ScriptObject.cpp Lexer.h Parser.h Script.h ScriptObject.h)
