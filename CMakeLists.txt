cmake_minimum_required(VERSION 3.26)
project(re_crack_v8_bytecode)

set(CMAKE_CXX_STANDARD 17)

include_directories(./v8/v8/include)
add_executable(re_crack_v8_bytecode v8dasm.cpp)

add_definitions(-DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX -D_ITERATOR_DEBUG_LEVEL=0)
#remove_definitions(-DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX)

set_property(TARGET re_crack_v8_bytecode PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_directories(re_crack_v8_bytecode PRIVATE "./v8/v8/out/x64.$<IF:$<CONFIG:DEBUG>,debug,release>.ptrc/obj")
target_link_libraries(re_crack_v8_bytecode v8_libbase.lib v8_libplatform.lib wee8.lib)
target_link_libraries(re_crack_v8_bytecode secur32.lib winmm.lib dmoguids.lib wmcodecdspuuid.lib msdmo.lib Strmiids.lib DbgHelp.lib)

