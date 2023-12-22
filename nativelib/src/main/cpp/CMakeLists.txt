# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

project("nativelib")

include_directories(
        Dobby/include
        Dobby/source
        Dobby/builtin-plugin
        Dobby/builtin-plugin/AndroidRestriction
        Dobby/builtin-plugin/SymbolResolver
        Dobby/external/logging
)
macro(SET_OPTION option value)
    set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()
# 关闭DobbyDebug日志
#SET_OPTION(DOBBY_DEBUG NO)
# 关闭shared库,和我们的so 集成到一起。
SET_OPTION(DOBBY_GENERATE_SHARED OFF)
#是否加载linker加载so的回调
SET_OPTION(Plugin.LinkerLoadCallback OFF)
add_subdirectory(Dobby dobby.build)


INCLUDE_DIRECTORIES(
        Dobby/include
        #dobby_c
)

add_library(${CMAKE_PROJECT_NAME} SHARED

        nativelib.cpp)


target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        dobby_static
        android
        log)