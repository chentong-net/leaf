function(leaf_configure_desktop_assets target_name assets_source_dir)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "leaf_configure_desktop_assets: target '${target_name}' does not exist")
    endif()

    get_filename_component(LEAF_ASSETS_SOURCE_DIR "${assets_source_dir}" ABSOLUTE)
    if(NOT EXISTS "${LEAF_ASSETS_SOURCE_DIR}")
        message(FATAL_ERROR "Leaf assets source directory does not exist: ${LEAF_ASSETS_SOURCE_DIR}")
    endif()

    set(LEAF_ASSETS_OUTPUT_DIR "$<TARGET_FILE_DIR:${target_name}>/assets")
    set(LEAF_ASSETS_PREPARE_TARGET "${target_name}-prepare-assets")

    add_custom_target(${LEAF_ASSETS_PREPARE_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Leaf assets: syncing ${LEAF_ASSETS_SOURCE_DIR} -> ${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${LEAF_ASSETS_SOURCE_DIR}" "${LEAF_ASSETS_OUTPUT_DIR}"
        VERBATIM
        COMMENT "Preparing Leaf desktop assets"
    )

    add_dependencies(${target_name} ${LEAF_ASSETS_PREPARE_TARGET})
endfunction()