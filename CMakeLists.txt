cmake_minimum_required(VERSION 3.21)

project(pipe_bender_repo)

# =========================
# C++ STANDARD
# =========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# QT
# =========================
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
qt_standard_project_setup()

# =========================
# OPENGL
# =========================
find_package(OpenGL REQUIRED)

# =========================
# SOURCES
# =========================
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
)

set(MACHINE_HEADERS
    Machine/MachineModel.h
    Machine/MachineState.h
)

# =========================
# GLAD (compiled OpenGL loader)
# =========================
add_library(glad STATIC
    external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/external/glad/include
)

target_link_libraries(glad PUBLIC OpenGL::GL)

# =========================
# EXECUTABLE
# =========================
qt_add_executable(pipe_bender_repo
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${RENDER_SOURCES}
    ${RENDER_HEADERS}
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${MACHINE_HEADERS}
)

# =========================
# INCLUDE PATHS
# =========================
target_include_directories(pipe_bender_repo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
)

# =========================
# LINK LIBRARIES
# =========================
target_link_libraries(pipe_bender_repo PRIVATE
    glad
    OpenGL::GL
    Qt6::Widgets
    Qt6::OpenGLWidgets
)