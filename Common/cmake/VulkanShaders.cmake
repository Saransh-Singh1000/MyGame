function(mygame_embed_vulkan_shaders target)
    set(shader_source_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    find_program(MYGAME_SHADER_COMPILER
        NAMES glslc glslc.exe glslangValidator glslangValidator.exe glslang glslang.exe
        HINTS
            "$ENV{VULKAN_SDK}/Bin"
            "$ENV{VULKAN_SDK}/bin"
    )

    set(shader_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")
    file(MAKE_DIRECTORY "${shader_dir}")
    set(shader_outputs)

    foreach(shader scene.vert scene.frag)
        string(REPLACE "." "_" symbol "${shader}")
        set(shader_source "${shader_source_dir}/shaders/${shader}")
        set(precompiled_shader "${shader_source}.spv")
        set(shader_dependencies "${precompiled_shader}")
        set(shader_compile_step)

        if(MYGAME_SHADER_COMPILER)
            set(spv_file "${shader_dir}/${shader}.spv")
            get_filename_component(shader_compiler_name "${MYGAME_SHADER_COMPILER}" NAME)
            if(shader_compiler_name MATCHES "^glslc")
                set(shader_compile_step
                    COMMAND "${MYGAME_SHADER_COMPILER}" "${shader_source}" -o "${spv_file}")
            else()
                set(shader_compile_step
                    COMMAND "${MYGAME_SHADER_COMPILER}" -V "${shader_source}" -o "${spv_file}")
            endif()
            list(PREPEND shader_dependencies "${shader_source}")
        else()
            set(spv_file "${precompiled_shader}")
        endif()

        set(header_file "${shader_dir}/${symbol}.hpp")
        add_custom_command(
            OUTPUT "${header_file}"
            ${shader_compile_step}
            COMMAND "${CMAKE_COMMAND}"
                "-DINPUT=${spv_file}"
                "-DOUTPUT=${header_file}"
                "-DSYMBOL=${symbol}"
                -P "${shader_source_dir}/cmake/EmbedSpirv.cmake"
            DEPENDS
                ${shader_dependencies}
                "${shader_source_dir}/cmake/EmbedSpirv.cmake"
            VERBATIM
        )
        list(APPEND shader_outputs "${header_file}")
    endforeach()

    add_custom_target(${target}VulkanShaders DEPENDS ${shader_outputs})
    add_dependencies(${target} ${target}VulkanShaders)
    target_include_directories(${target} PRIVATE "${shader_dir}")
endfunction()
