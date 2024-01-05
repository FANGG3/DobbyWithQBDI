# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)
project("nativelib")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)

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
SET_OPTION(DOBBY_DEBUG OFF)
# 关闭shared库,和我们的so 集成到一起。
SET_OPTION(DOBBY_GENERATE_SHARED OFF)
#是否加载linker加载so的回调
SET_OPTION(Plugin.LinkerLoadCallback ON)
SET_OPTION(Plugin.Android.BionicLinkerUtil ON)

add_subdirectory(Dobby dobby.build)


INCLUDE_DIRECTORIES(
        Dobby/include
        qdbi-arm64/include
        include
        utils
        hook
        record
        #dobby_c
)

# find local Installed QBDI
set(CMAKE_FIND_ROOT_PATH "/usr/local")

find_package(QBDIAARCH64 REQUIRED)

add_library(qdbi-arm64 STATIC IMPORTED)
add_library(qdbi-arm64-share SHARED IMPORTED)
set_target_properties(qdbi-arm64 PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/qdbi-arm64/lib/libQBDI.a)
set_target_properties(qdbi-arm64-share PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/qdbi-arm64/libQBDI.so)


aux_source_directory(hook hook_src_list)
aux_source_directory(utils utils_src_list)
aux_source_directory(record record_src_list)

add_library(${CMAKE_PROJECT_NAME} SHARED
        nativelib.cpp
        vm.cpp
        ${hook_src_list}
        ${utils_src_list}
        ${record_src_list}
        )


target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        dobby_static
        qdbi-arm64-share
#        QBDI::AARCH64::QBDI
        android
        log)