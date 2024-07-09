cmake_minimum_required(VERSION 3.14)
project(ILSQN LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置你的源文件和头文件
file(GLOB HEADERS "*.h" "*.hpp" "./include/*.hpp")
file(GLOB SRCS "*.cpp" "./include/*.cpp")

# 添加可执行文件
add_executable(ILSQN ${SRCS} ${HEADERS})

# 链接 Clipper 库
target_link_libraries(ILSQN)

# 如果需要使用 OpenMP，继续添加 OpenMP 的配置和链接部分
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(ILSQN PUBLIC OpenMP::OpenMP_CXX)
endif()
