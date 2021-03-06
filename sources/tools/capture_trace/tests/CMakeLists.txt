add_subdirectory(target)
add_subdirectory(payload)

set(CORE_MODULE_NAME "kedr")
set(CORE_MODULE "${CMAKE_BINARY_DIR}/core/${CORE_MODULE_NAME}.ko")

set(TRACE_MODULE_NAME "kedr_trace")
set(TRACE_MODULE "${CMAKE_BINARY_DIR}/trace/${TRACE_MODULE_NAME}.ko")

set(CAPTURE_TRACE_PROGRAM "${CMAKE_CURRENT_BINARY_DIR}/../${program_name}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test.sh"
    @ONLY
)

kedr_test_add_script("capture_trace.common.01"
    "${CMAKE_CURRENT_BINARY_DIR}/test.sh")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_session.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test_session.sh"
    @ONLY
)

kedr_test_add_script("capture_trace.session.01"
    "${CMAKE_CURRENT_BINARY_DIR}/test_session.sh")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_block.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test_block.sh"
    @ONLY
)

kedr_test_add_script("capture_trace.block.01"
    "${CMAKE_CURRENT_BINARY_DIR}/test_block.sh")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_block_session.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test_block_session.sh"
    @ONLY
)

kedr_test_add_script("capture_trace.block_session.01"
    "${CMAKE_CURRENT_BINARY_DIR}/test_block_session.sh")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_processing_trace.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test_processing_trace.sh"
    @ONLY
)

kedr_test_add_script("capture_trace.processing_trace.01"
    "${CMAKE_CURRENT_BINARY_DIR}/test_processing_trace.sh")
