-- Partially based on https://github.com/jpcy/xatlas/blob/master/premake5.lua (search for MiMalloc)
project "MiMalloc"
    cppdialect "C++17"
    kind "StaticLib"
    --targetname "mimalloc"

    vpaths {
        ["Headers"] = "include/*.h",
		["Sources"] = "src/*.c",
		["*"] = "premake5.lua"
    }

    includedirs "include"
    files { -- mi_sources. Must be kept updated according to the original CMakeLists.txt!
        "src/stats.c",
        "src/random.c",
        "src/os.c",
        "src/bitmap.c",
        "src/arena.c",
        "src/segment-cache.c",
        "src/segment.c",
        "src/page.c",
        "src/alloc.c",
        "src/alloc-aligned.c",
        "src/alloc-posix.c",
        "src/heap.c",
        "src/options.c",
        "src/init.c"
    }

    links { "psapi", "shell32", "user32", "advapi32", "bcrypt" }

    defines { "MI_STATIC_LIB" }
  
    buildoptions { "/Zc:__cplusplus" }

--[[     filter "configurations:Release"
        defines { 'CMAKE_BUILD_TYPE="Release"' }

    filter "configurations:Debug"
        defines { 'CMAKE_BUILD_TYPE="Debug"' } ]]

