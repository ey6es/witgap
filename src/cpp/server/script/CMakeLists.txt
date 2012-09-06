set(HEADERS Evaluator.h)
set(SOURCES Evaluator.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-script ${SOURCES} ${HEADERS}
    Globals.cpp Lexer.cpp MacroTransformer.cpp Parser.cpp Script.cpp ScriptObject.cpp
    Globals.h Lexer.h MacroTransformer.h Parser.h Script.h ScriptObject.h)

