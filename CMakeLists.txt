cmake_minimum_required(VERSION 3.20)
project(zio LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 插件输出必须是共享库
add_library(${PROJECT_NAME} SHARED src/plugin.cpp)
find_package(Threads REQUIRED)
# 添加插件SDK头文件
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/pluginsdk/include
)
set(SOURCES src/plugin.cpp
src/audio_buffer.cpp
src/audio_recorder.cpp
src/file_writer.cpp)
# 插件必须导出C符号，避免C++ name mangling
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PLUGIN_EXPORTS
)
target_link_libraries(${PROJECT_NAME}
        Threads::Threads  # 链接线程库
)
# 插件在Linux上生成 .so，在Windows生成 .dll
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""  # 移除 lib 前缀
    OUTPUT_NAME "${PROJECT_NAME}"  # 输出 ts3recorder.dll/so
    # SUFFIX ".dll"  # Windows 下用 .dll，Linux会自动改成 .so
)
