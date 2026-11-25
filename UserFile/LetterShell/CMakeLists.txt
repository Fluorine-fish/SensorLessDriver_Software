set(LETTER_SHELL_SRC "${PROJECT_SOURCE_DIR}/UserLib/LetterShell/src")

add_library(letter_shell OBJECT
    ${LETTER_SHELL_SRC}/shell.c
    ${LETTER_SHELL_SRC}/shell_ext.c
    ${LETTER_SHELL_SRC}/shell_cmd_list.c
    ${LETTER_SHELL_SRC}/shell_companion.c
    ShellPort.c
    ShellReply.c
    Commands/ShellSystemCmd.c
    Commands/ShellConfigCmd.c
)

target_include_directories(letter_shell PUBLIC
    ${LETTER_SHELL_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(letter_shell PRIVATE
    ${PROJECT_SOURCE_DIR}/UserFile/Bsp
    ${PROJECT_SOURCE_DIR}/UserFile/Comm
    ${PROJECT_SOURCE_DIR}/UserFile/Foc
    ${PROJECT_SOURCE_DIR}/UserFile/Application
)
target_compile_definitions(letter_shell PUBLIC
    SHELL_CFG_USER="Shell_cfg_user.h"
)
target_link_libraries(letter_shell PRIVATE stm32cubemx)
target_compile_options(letter_shell PRIVATE
    -fno-strict-aliasing
    "$<$<CONFIG:Debug>:-O2>"
    "$<$<CONFIG:Debug>:-g3>"
)
