add_rules("mode.release", "mode.debug") 
target("linqueue_test")
    set_kind("binary")
    add_files("queue/*.c", "./*.c")
    add_includedirs("./queue/", "/usr/local/include") 
    add_syslinks("pthread")
    add_cflags("-g", "-O2", "-DDEBUG")


