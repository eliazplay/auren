include_guard(GLOBAL)

function(auren_apply_project_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE
            /permissive-
            /Zc:__cplusplus
            /utf-8
            /W4
            /wd4100
            /wd4244
            /wd4267
        )
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wno-unused-parameter
        )
    endif()

    set_property(TARGET ${target_name} PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endfunction()

