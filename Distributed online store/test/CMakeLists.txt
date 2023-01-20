project(project3)

add_executable(run_vendors vendor.cc run_vendors.cc)
target_link_libraries(run_vendors 
	Threads::Threads
       	gRPC::grpc++
	p3protolib)
add_dependencies(run_vendors p3protolib)

add_executable(run_tests client.cc run_tests.cc product_queries_util.h)
target_link_libraries(run_tests 
	Threads::Threads
       	gRPC::grpc++
	p3protolib)
add_dependencies(run_tests p3protolib)

set_target_properties(run_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set_target_properties(run_vendors PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

#product_query_list.txt file symlink rule
set (source "${CMAKE_CURRENT_SOURCE_DIR}/product_query_list.txt")
set (destination "${CMAKE_BINARY_DIR}/bin/product_query_list.txt")
add_custom_command(
        TARGET run_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link ${destination} created."
)

