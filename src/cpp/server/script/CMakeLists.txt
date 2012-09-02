set(HEADERS )
set(SOURCES )

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-script ${SOURCES} ${HEADERS}
    Evaluator.cpp Globals.cpp Lexer.cpp Parser.cpp Script.cpp ScriptObject.cpp
    Evaluator.h Globals.h Lexer.h Parser.h Script.h ScriptObject.h)
