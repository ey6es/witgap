set(HEADERS Button.h Component.h Label.h Menu.h ScrollingList.h TextField.h Window.h)
set(SOURCES Button.cpp Component.cpp Label.cpp Menu.cpp ScrollingList.cpp TextField.cpp Window.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-ui ${SOURCES} ${HEADERS} Layout.cpp Layout.h Border.cpp Border.h)
