# Build widgets opcodes
if(APPLE)
        option(USE_FLTK "Use FLTK for graphs and widget opcodes" OFF)
else()
        option(USE_FLTK "Use FLTK for graphs and widget opcodes" ON)
endif()

set(FLTK_SKIP_OPENGL true)
find_package(FLTK)

check_deps(USE_FLTK FLTK_FOUND)
if(USE_FLTK)
    set(widgets_SRCS FL_graph.cpp winFLTK.c widgets.cpp)
    make_plugin(widgets "${widgets_SRCS}" "${FLTK_LIBRARIES}")
    target_include_directories(widgets PRIVATE ${CSOUND_INCLUDE_DIRS})
    if(NOT MSVC)
        target_link_libraries(widgets ${FLTK_BASE_LIBRARY})
        target_link_libraries(widgets ${FLTK_FORMS_LIBRARY})
        target_link_libraries(widgets ${FLTK_IMAGES_LIBRARY})
    else()
        target_link_libraries(widgets ${FLTK_LIBRARIES})
    endif()
    target_include_directories(widgets PRIVATE ${FLTK_INCLUDE_DIR})
endif()

if(USE_FLTK)
    set(virtual_SRCS virtual_keyboard/FLTKKeyboard.cpp
        virtual_keyboard/FLTKKeyboardWindow.cpp
        virtual_keyboard/FLTKKeyboardWidget.cpp
        virtual_keyboard/virtual_keyboard.cpp
        virtual_keyboard/Bank.cpp
        virtual_keyboard/KeyboardMapping.cpp
        virtual_keyboard/Program.cpp
        virtual_keyboard/SliderBank.cpp
        virtual_keyboard/SliderData.cpp)
    make_plugin(virtual "${virtual_SRCS}" "${FLTK_LIBRARIES}")
    if (NOT MSVC)
        target_link_libraries(virtual ${FLTK_BASE_LIBRARY})
        target_link_libraries(virtual ${FLTK_FORMS_LIBRARY})
        target_link_libraries(virtual ${FLTK_IMAGES_LIBRARY})
    else()
        target_link_libraries(virtual ${FLTK_LIBRARIES})
    endif()
    target_include_directories(virtual PRIVATE ${CSOUND_INCLUDE_DIRS})
    target_include_directories(virtual PRIVATE ".")
    target_include_directories(virtual PRIVATE ${FLTK_INCLUDE_DIR})
endif()

message(STATUS "")
