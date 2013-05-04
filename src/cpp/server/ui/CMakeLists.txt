set(HEADERS Button.h Component.h Label.h Menu.h ObjectEditor.h ResourceChooserDialog.h
    ScrollPane.h ScrollingList.h TabbedPane.h TextComponent.h Window.h)
set(SOURCES Button.cpp Component.cpp Label.cpp Menu.cpp ObjectEditor.cpp ResourceChooserDialog.cpp
    ScrollPane.cpp ScrollingList.cpp TabbedPane.cpp TextComponent.cpp Window.cpp)

qt4_wrap_cpp(SOURCES ${HEADERS})
add_library(server-ui ${SOURCES} ${HEADERS} Layout.cpp Layout.h Border.cpp Border.h)
