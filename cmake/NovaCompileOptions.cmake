# Shared compile definitions / options for Nova targets.
# Applied once from the root CMakeLists.txt.

include(CheckIPOSupported)

check_ipo_supported(RESULT NOVA_IPO_SUPPORTED OUTPUT NOVA_IPO_ERROR)

# ---------------------------------------------------------------------------
# Platform macros (propagated to all consumers of Nova-Core)
# ---------------------------------------------------------------------------
if(WIN32)
    set(NOVA_PLATFORM_DEFINE NOVA_WINDOWS)
elseif(APPLE)
    set(NOVA_PLATFORM_DEFINE NOVA_MACOS)
elseif(UNIX)
    set(NOVA_PLATFORM_DEFINE NOVA_LINUX)
else()
    set(NOVA_PLATFORM_DEFINE NOVA_UNKNOWN_PLATFORM)
endif()

# ---------------------------------------------------------------------------
# Build-type macros + platform (PUBLIC so headers like Log.h / Platform.h see them)
# ---------------------------------------------------------------------------
function(nova_apply_build_definitions target)
    target_compile_definitions(${target}
        PUBLIC
            ${NOVA_PLATFORM_DEFINE}
            $<$<CONFIG:Debug>:NOVA_DEBUG>
            $<$<CONFIG:Release>:NOVA_RELEASE>
            $<$<CONFIG:RelWithDebInfo>:NOVA_RELWITHDEBINFO>
            $<$<CONFIG:MinSizeRel>:NOVA_MINSIZEREL>
    )
endfunction()

# ---------------------------------------------------------------------------
# Runtime-oriented optimizations per configuration (cross-compiler)
# ---------------------------------------------------------------------------
function(nova_apply_optimization_options target)
    if(MSVC)
        target_compile_options(${target}
            PRIVATE
                $<$<CONFIG:Release>:/O2>
                $<$<CONFIG:Release>:/Ob2>
                $<$<CONFIG:Release>:/Ot>
                $<$<CONFIG:RelWithDebInfo>:/O2>
                $<$<CONFIG:RelWithDebInfo>:/Ob2>
                $<$<CONFIG:MinSizeRel>:/O1>
                $<$<CONFIG:MinSizeRel>:/Ob1>
        )
        target_link_options(${target}
            PRIVATE
                $<$<CONFIG:Release>:/LTCG>
                $<$<CONFIG:MinSizeRel>:/LTCG>
        )
        target_compile_options(${target}
            PRIVATE
                $<$<CONFIG:Release>:/GL>
                $<$<CONFIG:MinSizeRel>:/GL>
        )
    else()
        target_compile_options(${target}
            PRIVATE
                $<$<CONFIG:Release>:-O3>
                $<$<CONFIG:RelWithDebInfo>:-O2>
                $<$<CONFIG:MinSizeRel>:-Os>
        )
    endif()

    if(NOVA_IPO_SUPPORTED)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
    endif()
endfunction()

# Executables land in Bin/<Config>/ (Debug, Release, ...).
# Works for both multi-config (VS, Ninja Multi-Config) and single-config generators.
function(nova_set_runtime_output target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${NOVA_BIN_DIR}/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${NOVA_BIN_DIR}/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${NOVA_BIN_DIR}/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${NOVA_BIN_DIR}/RelWithDebInfo"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${NOVA_BIN_DIR}/MinSizeRel"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/"
    )
endfunction()

function(nova_configure_target target)
    nova_apply_build_definitions(${target})
    nova_apply_optimization_options(${target})
endfunction()