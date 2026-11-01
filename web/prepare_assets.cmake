if(NOT DEFINED LEAF_WEB_ASSET_SOURCE_DIR OR
   NOT DEFINED LEAF_WEB_ASSET_OUTPUT_DIR)
    message(FATAL_ERROR "Web asset source and output directories are required")
endif()

if(NOT IS_DIRECTORY "${LEAF_WEB_ASSET_SOURCE_DIR}")
    message(FATAL_ERROR
            "Web asset source directory does not exist: ${LEAF_WEB_ASSET_SOURCE_DIR}")
endif()

file(REMOVE_RECURSE "${LEAF_WEB_ASSET_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${LEAF_WEB_ASSET_OUTPUT_DIR}")
file(COPY "${LEAF_WEB_ASSET_SOURCE_DIR}/"
        DESTINATION "${LEAF_WEB_ASSET_OUTPUT_DIR}"
        PATTERN "fonts" EXCLUDE
        PATTERN ".DS_Store" EXCLUDE)
