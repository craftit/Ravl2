
# A very simple example of a configured file that might need to be
# converted to one that is publicly installed in the case that
# you are developing a library
configure_file("config.hh.in" "${CMAKE_BINARY_DIR}/include/internal_use_only/config.hh" ESCAPE_QUOTES)


