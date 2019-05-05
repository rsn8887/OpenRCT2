# Helpers for linking platform specific libraries

function(target_link_platform_libraries target)

if (APPLE)
    target_link_libraries(${target} "-framework Cocoa")
elseif(WIN32)
    target_link_libraries(${target} gdi32)
elseif(SWITCH_BUILD)
    target_link_libraries(${target} icuuc icudata icui18n)
endif ()

endfunction()
