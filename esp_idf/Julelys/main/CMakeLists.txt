idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
)

execute_process(COMMAND ../js2h.sh ../js_files/rain.js ../main/js_string.h)