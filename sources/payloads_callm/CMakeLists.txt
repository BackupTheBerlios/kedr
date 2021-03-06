# Additional symbols to use. Note that the symbols from KEDR Core are
# made available to the payload modules automatically if 
# kedr_create_payload_module() is used to create them.
# The dependency on KEDR Core is also set automatically for such modules.
kbuild_use_symbols("${CMAKE_BINARY_DIR}/trace/Module.symvers")
kbuild_add_dependencies("kedr_trace")

# The names of the main data file and of the file containing the 
# payload-specific portion of its header.
set(payload_data_file "payload.data")
set(header_data_file "header.data")

# The commands to create a payload module for call monitoring.
function(create_payload_callm module_name functions)
	kedr_create_payload_module(${module_name} ${payload_data_file} 
		"${KEDR_GEN_TEMPLATES_DIR}/payload_callm.c/")
		
	# Rules to prepare the full data file for the payload module
	kedr_create_payload_data(${header_data_file} ${payload_data_file}
		${functions} ${ARGN})
endfunction(create_payload_callm module_name functions)

add_subdirectory(common_mm)
add_subdirectory(mutexes)
add_subdirectory(spinlocks)
add_subdirectory(uaccess)
add_subdirectory(schedule)
add_subdirectory(waitqueue)
add_subdirectory(capable)
add_subdirectory(virtual_mm)
add_subdirectory(mem_util)

kedr_test_add_subdirectory(tests)
