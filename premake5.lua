workspace "FortCmd"
    configurations { "Release" }

project "FortCmd"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"
    targetdir "bin"
    objdir "obj"
    files {
        "src/*.cpp",

        "minhook/src/**.c",
    }
    includedirs {
        "minhook/include"
    }
    buildoptions { "/wd4369", "/wd4309" }
    architecture "x64"
