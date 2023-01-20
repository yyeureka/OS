# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(project3)

file(GLOB SRCS *.cc *.h)
add_executable(store ${SRCS})
target_link_libraries(store 
	Threads::Threads
       	gRPC::grpc++
	p3protolib)
add_dependencies(store p3protolib)
set_target_properties(store PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

#vendor_addresses.txt file symlink rule
set (source "${CMAKE_CURRENT_SOURCE_DIR}/vendor_addresses.txt")
set (destination "${CMAKE_BINARY_DIR}/bin/vendor_addresses.txt")
add_custom_command(
        TARGET store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link ${destination} created."
)
