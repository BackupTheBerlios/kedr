# Simply copy scripts into binary tree

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/do_commands.sh"
    "${CMAKE_CURRENT_BINARY_DIR}/do_commands.sh"
    @ONLY)
