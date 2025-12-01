require('config_utils')

is_real_bot = false

-- num_push_up must be an integer greater than 0
num_push_up = 3             
-- For safety reasons, push_up_time_scale cannot be less than 0.5        
push_up_time_scale = 0.8            

local graph = require('common_graph_define')
-- local graph = require('common_graph_define_test_para_mech')

graph.init(is_real_bot)

local options = require('common_module_options')
-- local options = require('common_module_options_test_para_mech')


options.init(is_real_bot)
options.resetPushUp(num_push_up, push_up_time_scale)

values = {
    options = options,
    graph = graph,
}


-- 版本迭代

-- if !is_real_bot or /opt/booster/robot_info.txt not found, default_version working
local default_version = "2.3.2"

if VersionGreaterOrEquals(default_version, "2.3.2", is_real_bot) then
    options.custom_traj.traj_4 = stand_up_face_down_traj
    print("versionGreaterOrEquals 2.3.2")
end


return values
