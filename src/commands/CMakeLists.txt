# Source files
set (CMD_SRCS ${CMD_SRCS} "Command.cpp" "../../include/commands/Command.h")
set (CMD_SRCS ${CMD_SRCS} "CommandBind.cpp" "../../include/commands/CommandBind.h")
set (CMD_SRCS ${CMD_SRCS} "CommandEngine.cpp" "../../include/commands/CommandEngine.h")
set (CMD_SRCS ${CMD_SRCS} "CommandSet.cpp" "../../include/commands/CommandSet.h")

# Add library
add_library (OpenRaider_commands OBJECT ${CMD_SRCS})

