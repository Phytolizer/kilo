workspace("kilo")
    configurations({"Debug", "Release"})

project("kilo")
    kind("ConsoleApp")
    language("C")
    files({"kilo.c"})

    filter({"configurations:Debug"})
        defines({"DEBUG"})
        symbols("On")

    filter({"configurations:Release"})
        defines({"NDEBUG"})
        optimize("On")
