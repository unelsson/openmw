set(OSGQT_LIBRARY "osgQt")


SET(SOURCE_H
    osgQOpenGLWidget
    osgQOpenGLWindow
    OSGRenderer
)

qt5_wrap_cpp(SOURCES_H_MOC ${SOURCE_H} #[[OPTIONS ${MOC_OPTIONS}]])

# Sources

set(OSGQT_SOURCE_FILES
    GraphicsWindowQt.cpp
    CullVisitorEx.cpp
    osgQOpenGLWidget.cpp
    osgQOpenGLWindow.cpp
    OSGRenderer.cpp
    RenderStageEx.cpp
    StateEx.cpp
    ${SOURCES_H_MOC}
)

add_library(${OSGQT_LIBRARY} STATIC ${OSGQT_SOURCE_FILES})

    target_link_libraries(${OSGQT_LIBRARY} Qt5::Core Qt5::Gui Qt5::OpenGL)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EXTERN_OSGQT_LIBRARY ${OSGQT_LIBRARY} PARENT_SCOPE)
