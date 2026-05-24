cmake_minimum_required(VERSION 3.21)

project(pipe_bender_repo)

# =========================================================
# C++ STANDARD
# =========================================================
# Modern OpenGL + Qt6 requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# QT MODULES
# =========================================================
# Widgets = UI
# OpenGLWidgets = QOpenGLWidget support (your GLView)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

qt_standard_project_setup()

# =========================================================
# OPENGL (system library)
# =========================================================
# Provides opengl32.lib on Windows
find_package(OpenGL REQUIRED)

# =========================================================
# PROJECT SOURCES
# =========================================================

set(APP_SOURCES
    App/mainQT.cpp
    App/MainWindow.cpp
    App/AppController.cpp
)

set(APP_HEADERS
    App/MainWindow.h
    App/AppController.h
)

set(RENDER_SOURCES
    Render/GLView.cpp
    Source/PipeRenderer.cpp
    Source/HUDPanel.cpp
    Source/CameraGL.cpp
    Source/ControlCamera.cpp
    Source/ShaderGL.cpp
    Source/ShaderManager.cpp
    Source/TubeMesh.cpp
)

set(RENDER_HEADERS
    Render/GLView.h
    Render/PipeRenderer.h
    Render/HUDPanel.h
    Render/CameraGL.h
    Render/ControlCamera.h
    Render/ShaderGL.h
    Render/ShaderManager.h
    Render/TubeMesh.h
)

set(CORE_SOURCES
    Source/SimulationController.cpp
    Source/ProgramLoader.cpp
)

set(CORE_HEADERS
    Core/SimulationController.h
    Core/PipeAxis3D.h
    Core/PipeAxis2D.h
    Core/PipeAxis3D_v2.h
    Core/Operations.h
    Core/OperationQueue.h
    Core/ProgramLoader.h
    Core/Math/Vec2D.h
    Core/Math/Vec3D.h
    Core/BendDirection.h
    Core/RotationDirection.h
    Core/Geometry/Frame.h
    Core/Geometry/PipeNode.h
    Core/Geometry/PipeSegment.h
)

set(MACHINE_HEADERS
    Machine/MachineModel.h
    Machine/MachineState.h
)

# =========================================================
# GLAD (OpenGL loader - compiled library)
# =========================================================
#OpenGL functions are NOT linked directly on Windows
add_library(glad STATIC
    external/glad/src/gl.c
 )

# Make GLAD headers visible to all users of this library
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

# IMPORTANT:
# GLAD itself does NOT require linking OpenGL
# (it only loads function pointers at runtime)
# So we REMOVE OpenGL dependency from here

# =========================================================
# EXECUTABLE
# =========================================================
qt_add_executable(pipe_bender_repo
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${RENDER_SOURCES}
    ${RENDER_HEADERS}
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${MACHINE_HEADERS}
)

# =========================================================
# INCLUDE PATHS
# =========================================================
target_include_directories(pipe_bender_repo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # project root includes
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm  # GLM header-only library
    ${CMAKE_SOURCE_DIR}/Common
)

# =========================================================
# LINK LIBRARIES (FINAL APP ONLY)
# =========================================================
# Order matters less in modern CMake, but grouping is clean
target_link_libraries(pipe_bender_repo PRIVATE
    glad                     # OpenGL loader
    OpenGL::GL              # system OpenGL
    Qt6::Widgets            # UI
    Qt6::OpenGLWidgets      # QOpenGLWidget
)