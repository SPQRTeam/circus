package.path = package.path .. ";../src/tests/?.lua"

is_real_bot = false

-- local graph = require('common_graph_define_v23_rl_isaac')
local graph = require('common_graph_define_t1_rl_isaac')
-- local graph = require('common_graph_define_t1_rl_robocup_isaac')


graph.init(is_real_bot)

-- local options = require('common_module_options_v23_rl_isaac')
local options = require('common_module_options_t1_rl_isaac')
-- local options = require('common_module_options_t1_rl_robocup_isaac')

options.init(is_real_bot)

options.command_manager.robot_name = "t1_blue_1"
options.command_manager.joystick_index = 4

values = {
    options = options,
    graph = graph,
}

return values
