    function(mygame_compile_vulkan_shaders target)
    set(shader_source_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    find_program(MYGAME_GLSLC_EXECUTABLE
        NAMES glslc glslc.exe
        HINTS
            "$ENV{VULKAN_SDK}/Bin"
            "$ENV{VULKAN_SDK}/bin"
    )
    if(NOT MYGAME_GLSLC_EXECUTABLE)
        find_program(MYGAME_GLSLANG_VALIDATOR_EXECUTABLE
            NAMES glslangValidator glslangValidator.exe
            HINTS
                "$ENV{VULKAN_SDK}/Bin"
                "$ENV{VULKAN_SDK}/bin"
        )
    endif()
    if(NOT MYGAME_GLSLC_EXECUTABLE AND NOT MYGAME_GLSLANG_VALIDATOR_EXECUTABLE)
        message(FATAL_ERROR
            "glslc or glslangValidator is required to build the Vulkan renderer. "
            "Install a shader compiler and ensure it is available on PATH.")
    endif()

    set(shader_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")
    file(MAKE_DIRECTORY "${shader_dir}")
    set(shader_outputs)

    foreach(shader scene.vert scene.frag)
        string(REPLACE "." "_" symbol "${shader}")
        set(spv_file "${shader_dir}/${shader}.spv")
        set(header_file "${shader_dir}/${symbol}.hpp")
        if(MYGAME_GLSLC_EXECUTABLE)
            set(shader_compile_command
                "${MYGAME_GLSLC_EXECUTABLE}"
                "${shader_source_dir}/shaders/${shader}"
                -o "${spv_file}"
            )
        else()
            set(shader_compile_command
                "${MYGAME_GLSLANG_VALIDATOR_EXECUTABLE}"
                -V "${shader_source_dir}/shaders/${shader}"
                -o "${spv_file}"
            )
        endif()
        add_custom_command(
            OUTPUT "${header_file}"
            COMMAND ${shader_compile_command}
            COMMAND "${CMAKE_COMMAND}"
                "-DINPUT=${spv_file}"
                "-DOUTPUT=${header_file}"
                "-DSYMBOL=${symbol}"
                -P "${shader_source_dir}/cmake/EmbedSpirv.cmake"
            DEPENDS
                "${shader_source_dir}/shaders/${shader}"
                "${shader_source_dir}/cmake/EmbedSpirv.cmake"
            VERBATIM
        )
        list(APPEND shader_outputs "${header_file}")
    endforeach()

    add_custom_target(${target}VulkanShaders DEPENDS ${shader_outputs})
    add_dependencies(${target} ${target}VulkanShaders)
    target_include_directories(${target} PRIVATE "${shader_dir}")
endfunction()
