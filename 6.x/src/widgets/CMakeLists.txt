option(USE_FLTK "Use FLTK for graphs and widget opcodes" ON)

set(FLTK_SKIP_OPENGL true)

if(USE_VCPKG)
    find_package(FLTK CONFIG)
else()
    find_package(FLTK)
endif()

check_deps(USE_FLTK FLTK_FOUND)

if(USE_FLTK)
    set(widgets_SRCS FL_graph.cpp winFLTK.c widgets.cpp)

    make_plugin(widgets "${widgets_SRCS}" "${FLTK_LIBRARIES}")
    target_include_directories(widgets PRIVATE ${FLTK_INCLUDE_DIR})

    if(USE_VCPKG)
        target_link_libraries(widgets PRIVATE fltk fltk_gl fltk_forms fltk_images)
    else()
        target_link_libraries(widgets PRIVATE
            ${FLTK_BASE_LIBRARY} ${FLTK_FORMS_LIBRARY} ${FLTK_IMAGES_LIBRARY}
        )
    endif()
endif()

if(USE_FLTK)
    set(virtual_SRCS
        virtual_keyboard/FLTKKeyboard.cpp
        virtual_keyboard/FLTKKeyboardWindow.cpp
        virtual_keyboard/FLTKKeyboardWidget.cpp
        virtual_keyboard/virtual_keyboard.cpp
        virtual_keyboard/Bank.cpp
        virtual_keyboard/KeyboardMapping.cpp
        virtual_keyboard/Program.cpp
        virtual_keyboard/SliderBank.cpp
        virtual_keyboard/SliderData.cpp)

    make_plugin(virtual "${virtual_SRCS}" "${FLTK_LIBRARIES}")
    target_include_directories(virtual PRIVATE ${FLTK_INCLUDE_DIR} ".")

    if (USE_VCPKG)
        target_link_libraries(virtual PRIVATE fltk fltk_gl fltk_forms fltk_images)
    else()
        target_link_libraries(virtual PRIVATE
            ${FLTK_BASE_LIBRARY} ${FLTK_FORMS_LIBRARY} ${FLTK_IMAGES_LIBRARY}
        )
    endif()
endif()
