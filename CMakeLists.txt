cmake_minimum_required(VERSION 3.10)
project(chplus_interpreter)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(MSVC)
    # MSVC编译器选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    # GCC/Clang编译器选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 查找所有源文件
file(GLOB_RECURSE SOURCE_FILES
    "src/lexer/*.cpp"
    "src/parser/*.cpp"
    "src/executor/*.cpp"
    "src/utils/*.cpp"
    "main.cpp"
)

# 创建可执行文件
add_executable(chplus ${SOURCE_FILES})

# 链接库（如果需要的话）
# target_link_libraries(chplus ${LIBS})

# 设置输出目录
set_target_properties(chplus PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 打印配置信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source Files:")
foreach(SOURCE_FILE ${SOURCE_FILES})
    message(STATUS "  ${SOURCE_FILE}")
endforeach()

# 可选：创建调试和发布配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(chplus PRIVATE DEBUG)
    message(STATUS "Build Type: Debug")
else()
    message(STATUS "Build Type: Release")
endif()