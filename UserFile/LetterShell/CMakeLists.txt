set(LETTER_SHELL_SRC "${PROJECT_SOURCE_DIR}/UserLib/LetterShell/src")

add_library(vesc_letter_shell OBJECT
    ${LETTER_SHELL_SRC}/shell.c
    ${LETTER_SHELL_SRC}/shell_ext.c
    ${LETTER_SHELL_SRC}/shell_cmd_list.c
    ${LETTER_SHELL_SRC}/shell_companion.c
    ShellPort.c
    ShellReply.c
    Commands/ShellSystemCmd.c
)

target_include_directories(vesc_letter_shell PUBLIC
    ${LETTER_SHELL_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(vesc_letter_shell PRIVATE
    ${PROJECT_SOURCE_DIR}/UserFile/Bsp
)
target_compile_definitions(vesc_letter_shell PUBLIC
    SHELL_CFG_USER="Shell_cfg_user.h"
)
target_link_libraries(vesc_letter_shell PRIVATE stm32cubemx)
target_compile_options(vesc_letter_shell PRIVATE
    -fno-strict-aliasing
    "$<$<CONFIG:Debug>:-O2>"
    "$<$<CONFIG:Debug>:-g3>"
)
