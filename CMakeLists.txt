cmake_minimum_required(VERSION 3.10)
project(ILSQN LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认 Release 编译优化
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# 设置你的源文件和头文件（核心算法）
file(GLOB HEADERS "*.h" "*.hpp" "./include/*.hpp" "libnfporb-master/src/algo/*.hpp" "libnfporb-master/src/*.hpp")
file(GLOB SRCS "*.cpp" "./include/*.cpp")

# 添加命令行可执行文件
add_executable(ILSQN ${SRCS} ${HEADERS})

# OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(ILSQN PUBLIC OpenMP::OpenMP_CXX)
endif()

# ===== Qt5 GUI（可选） =====
option(BUILD_QT_GUI "Build Qt5 GUI visualizer" OFF)

if(BUILD_QT_GUI)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)

    # Qt GUI 源文件
    file(GLOB QT_SRCS "qt/*.cpp")
    file(GLOB QT_HEADERS "qt/*.h")

    # 核心算法源文件（排除 main.cpp，用 qt/main_qt.cpp 代替）
    file(GLOB CORE_SRCS "*.cpp" "./include/*.cpp")
    list(FILTER CORE_SRCS EXCLUDE REGEX "main\\.cpp$")

    add_executable(ILSQN_GUI ${QT_SRCS} ${QT_HEADERS} ${CORE_SRCS} ${HEADERS})
    target_link_libraries(ILSQN_GUI PRIVATE Qt5::Widgets)

    if(OpenMP_CXX_FOUND)
        target_link_libraries(ILSQN_GUI PUBLIC OpenMP::OpenMP_CXX)
    endif()
endif()

