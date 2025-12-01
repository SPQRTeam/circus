
local graph = {
    import = {
        commander1 = "lib/libmodule_source.so",
        rs_sdk = "lib/librs_sdk_interface.so",
        parallel_mech_input = "lib/libparallel_mech.so",
        parallel_mech_output = "lib/libparallel_mech.so",
        sw1 = "lib/libspecial.so",
        common_legged_ekf = "lib/libstate_estimate.so",
        legged_estimate_convert = "lib/liblegged_estimate_convert.so",
        planner_dcm = "lib/libbiped_dcm_planner.so",
        hand_planner = "lib/libbiped_planner.so",
        hand_direct_command_planner = "lib/libbiped_planner.so",
        planner_wbc_convert = "lib/libplanner_wbc_convert.so",
        wbc_common = "lib/libcommon_wbc.so",
        pvt = "lib/libpvt.so",
        module_publisher = "lib/libmodule_motion_state_publisher.so",
        module_rviz = "lib/libmodule_rviz_publisher.so",
        intercept_motor_cmd = "lib/libpvt_cmd_intercept.so",
        rl_locomotion = "lib/librl_locomotion.so",
        -- fall_down_recovery = "lib/libfall_down_recovery.so",
    },
    modules = {
        commander1 = "source::MitVariedTarget",
        command_manager = "source::CommandManagerV2",
        damping_mode = "source::DampingMode",
        debugging_mode = "source::DebuggingMode",
        simulator_out = "rs_sdk_interface::RsSdkOutput",
        simulator_in = "rs_sdk_interface::RsSdkInput",
        parallel_mech_input_stance = "parallel_mech::BipedParallelFeetInput",
        -- parallel_mech_input_rl = "parallel_mech::BipedParallelFeetInput",
        parallel_mech_input_custom_traj = "parallel_mech::BipedParallelFeetInput",
        -- parallel_mech_input_custom_mode = "parallel_mech::BipedParallelFeetInput",
        parallel_mech_output = "parallel_mech::BipedParallelFeetOutput",
        joint_map_output = "parallel_mech::JointMapOutput",
        joint_map_input = "parallel_mech::JointMapInput",
        planner_pvt_switch = "special::Switch",
        common_legged_ekf = "state_estimate::CommonLeggedEKF",
        legged_estimate_convert = "legged_estimate_convert::LeggedEstimateConvert",
        contact_probability_portal_collect = "special::PortalCollect",
        contact_probability_portal_publish = "special::PortalPublish",

        hand_planner = "biped_planner::BipedHandPlanner",
        hand_direct_command_planner = "biped_planner::BipedHandCommandPlanner",
        hand_direct_command_portal_collect = "special::PortalCollect",
        hand_direct_command_portal_publish = "special::PortalPublish",
        -- stance planner
        stance_planner = "biped_planner::BipedQuasiStaticPlanner",
        stance_planner_wbc_convert = "planner_wbc_convert::PlannerWbcConvert",
        stance_wbc_common = "whole_body_control::CommonWBC",
        stance_pvt = "pvt::PvtGenerator",
        stance_mode_portal_collect = "special::PortalCollect",
        stance_mode_portal_publish = "special::PortalPublish",

        publisher = "motion_state_publisher::MotionStatePublisher",
        rviz = "rviz_publisher::RvizPublisher",
        intercept_motor_cmd = "pvt_cmd_intercept::PvtCmdIntercept",

        rl_locomotion = "rl_locomotion::RMALocomotion",
        rl_locomotion_run = "rl_locomotion::RMARun",
        -- rl_locomotion_gait_2 = "rl_locomotion::RMARunMultiModal",
        -- rl_fall_recovery = "rl_locomotion::RMAFall",
        -- rl_locomotion_fallen_portal_collect = "special::PortalCollect",
        -- rl_locomotion_fallen_portal_publish = "special::PortalPublish",
        -- rl_locomotion_run_fallen_portal_collect = "special::PortalCollect",
        -- rl_locomotion_run_fallen_portal_publish = "special::PortalPublish",
        -- rl_locomotion_gait_2_fallen_portal_collect = "special::PortalCollect",
        -- rl_locomotion_gait_2_fallen_portal_publish = "special::PortalPublish",

        custom_traj = "source::NestedTrajectory",
        -- custom_traj_finished_portal_collect = "special::PortalCollect",
        -- custom_traj_finished_portal_publish = "special::PortalPublish",
        -- fall_down_recovery = "fall_down_recovery::FallDownRecovery",
        -- stand_up_type_idx_portal_collect = "special::PortalCollect",
        -- stand_up_type_idx_portal_publish = "special::PortalPublish",
        -- fdr_state_portal_collect = "special::PortalCollect",
        -- fdr_state_portal_publish = "special::PortalPublish",
        -- is_recovery_avalible_portal_collect = "special::PortalCollect",
        -- is_recovery_avalible_portal_publish = "special::PortalPublish",
    },
    connections = {
        -------------------------- connect to commander1 --------------------------
        con2 = {
            output_module = "joint_map_output",
            output_message_name = "joint_states_mapped",
            input_module = "commander1",
            input_message_name = "motor_state_feedback",
        },

        con_joint_states_serial_2_commander_manager = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "command_manager",
            input_message_name = "joint_states_serial",
        },
        con_portal_publish_module_a = {
            output_module = "stance_mode_portal_publish",
            output_message_name = "output",
            input_module = "command_manager",
            input_message_name = "current_stance_mode_index_",
        },
        con_hand_publish_module_a = {
            output_module = "hand_direct_command_portal_publish",
            output_message_name = "output",
            input_module = "command_manager",
            input_message_name = "hand_task_failed_id_",
        },
        -- con_portal_publish_module_b = {
        --     output_module = "rl_locomotion_fallen_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "rl_locomotion_fallen_",
        -- },
        -- con_portal_publish_module_c = {
        --     output_module = "rl_locomotion_run_fallen_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "rl_locomotion_run_fallen_",
        -- },
        -- con_portal_publish_module_d = {
        --     output_module = "rl_locomotion_gait_2_fallen_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "rl_locomotion_gait_2_fallen_",
        -- },
        -- con_cmd_manager_fall_down_recovery_state = {
        --     output_module = "fdr_state_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "fall_down_recovery_state",
        -- },
        -- con_cmd_manager_is_recovery_avalible = {
        --     output_module = "is_recovery_avalible_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "is_recovery_avalible",
        -- },
        -- con_custom_traj_finished = {
        --     output_module = "custom_traj_finished_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "custom_traj_finished",
        -- },
        -- con_stand_up_type_idx = {
        --     output_module = "stand_up_type_idx_portal_publish",
        --     output_message_name = "output",
        --     input_module = "command_manager",
        --     input_message_name = "stand_up_type_idx",
        -- },

        -------------------------- connect to damping_mode --------------------------
        con_pmo_dpm = {
            output_module = "joint_map_output",
            output_message_name = "joint_states_mapped",
            input_module = "damping_mode",
            input_message_name = "motor_state_feedback",
        },

        -------------------------- connect to debugging_mode --------------------------
        con_commander_manager_2_debugging = {
            output_module = "command_manager",
            output_message_name = "user_motor_pvt_cmd_",
            input_module = "debugging_mode",
            input_message_name = "user_motor_pvt_cmd_",
        },

        -------------------------- connect to simulator_out --------------------------
        -- None

        -------------------------- connect to simulator_in --------------------------
        con_commander_manager_2_rs_sdk_input = {
            output_module = "command_manager",
            output_message_name = "current_mode",
            input_module = "simulator_in",
            input_message_name = "current_mode",
        },
        con_jmi_1 = {
            output_module = "joint_map_input",
            output_message_name = "motor_pvt_commands_mapped",
            input_module = "simulator_in",
            input_message_name = "motor_pvt_commands",
        },

        -------------------------- connect to parallel_mech_input_stance --------------------------
        con_commander_manager_2_pmi_stance = {
            output_module = "command_manager",
            output_message_name = "is_parallel_input_",
            input_module = "parallel_mech_input_stance",
            input_message_name = "is_parallel_input",
        },
        con5_stance = {
            output_module = "stance_pvt",
            output_message_name = "motor_pvt_commands_serial",
            input_module = "parallel_mech_input_stance",
            input_message_name = "motor_pvt_commands_serial",
        },
        con_pi_1_stance = {
            output_module = "joint_map_output",
            output_message_name = "joint_states_mapped",
            input_module = "parallel_mech_input_stance",
            input_message_name = "joint_states_parallel",
        },
        con_pi_2_stance = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "parallel_mech_input_stance",
            input_message_name = "joint_states_serial",
        },

        -------------------------- connect to parallel_mech_input_rl --------------------------
        -- con_commander_manager_2_pmi_rl = {
        --     output_module = "command_manager",
        --     output_message_name = "is_parallel_input_",
        --     input_module = "parallel_mech_input_rl",
        --     input_message_name = "is_parallel_input",
        -- },
        -- con_rl_parallel = {
        --     output_module = "rl_locomotion_run",
        --     output_message_name = "mit_motor_command",
        --     input_module = "parallel_mech_input_rl",
        --     input_message_name = "motor_pvt_commands_serial",
        -- },
        -- con_pi_1_rl = {
        --     output_module = "joint_map_output",
        --     output_message_name = "joint_states_mapped",
        --     input_module = "parallel_mech_input_rl",
        --     input_message_name = "joint_states_parallel",
        -- },
        -- con_pi_rl_2 = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_states",
        --     input_module = "parallel_mech_input_rl",
        --     input_message_name = "joint_states_serial",
        -- }, 

        -------------------------- connect to parallel_mech_input_custom_mode --------------------------
        -- con_commander_manager_2_pmi_custom_mode = {
        --     output_module = "command_manager",
        --     output_message_name = "is_parallel_input_",
        --     input_module = "parallel_mech_input_custom_mode",
        --     input_message_name = "is_parallel_input",
        -- },

        -- con_custom_to_pmi_custom = {
        --     output_module = "debugging_mode",
        --     output_message_name = "mit_motor_command",
        --     input_module = "parallel_mech_input_custom_mode",
        --     input_message_name = "motor_pvt_commands_serial",
        -- },
        con_custom_mode_planner = {
            output_module = "debugging_mode",
            output_message_name = "mit_motor_command",
            input_module = "planner_pvt_switch",
            input_message_name = "in_4",
        },

        -- con_pi_1_custom_mode = {
        --     output_module = "joint_map_output",
        --     output_message_name = "joint_states_mapped",
        --     input_module = "parallel_mech_input_custom_mode",
        --     input_message_name = "joint_states_parallel",
        -- },
        -- con_pi_2_custom_mode = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_states",
        --     input_module = "parallel_mech_input_custom_mode",
        --     input_message_name = "joint_states_serial",
        -- },
        -- con_pi_2_rl_fall_recovery = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_states",
        --     input_module = "parallel_mech_input_rl_fall_recovery",
        --     input_message_name = "joint_states_serial",
        -- },

        -------------------------- connect to parallel_mech_input_custom_traj --------------------------
         con_commander_manager_2_pmi_custom_traj = {
            output_module = "command_manager",
            output_message_name = "is_parallel_input_",
            input_module = "parallel_mech_input_custom_traj",
            input_message_name = "is_parallel_input",
        },
        con5_custom_traj = {
            output_module = "custom_traj",
            output_message_name = "motor_command",
            input_module = "parallel_mech_input_custom_traj",
            input_message_name = "motor_pvt_commands_serial",
        },
        con_pi_1_custom_traj = {
            output_module = "joint_map_output",
            output_message_name = "joint_states_mapped",
            input_module = "parallel_mech_input_custom_traj",
            input_message_name = "joint_states_parallel",
        },
        con_pi_2_custom_traj = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "parallel_mech_input_custom_traj",
            input_message_name = "joint_states_serial",
        },

        -------------------------- connect to parallel_mech_output --------------------------
        con_po_1 = {
            output_module = "joint_map_output",
            output_message_name = "joint_states_mapped",
            input_module = "parallel_mech_output",
            input_message_name = "joint_states_parallel",
        },
        con_po_2 = {
            output_module = "joint_map_output",
            output_message_name = "joint_forces_mapped",
            input_module = "parallel_mech_output",
            input_message_name = "joint_forces_parallel",
        },

        -------------------------- connect to joint_map_output --------------------------
        con_jmo_1 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "joint_map_output",
            input_message_name = "joint_states_original",
        },

        con_jmo_2 = {
            output_module = "simulator_out",
            output_message_name = "joint_forces",
            input_module = "joint_map_output",
            input_message_name = "joint_forces_original",
        },

        -------------------------- connect to joint_map_input --------------------------
        con_intercept_motor_cmd_2_joint_map_input = {
            output_module = "intercept_motor_cmd",
            output_message_name = "motor_pvt_intercepted_",
            input_module = "joint_map_input",
            input_message_name = "motor_pvt_commands_original",
        },

        -------------------------- connect to planner_pvt_switch --------------------------
        con_idx2_a1 = {
            output_module = "command_manager",
            output_message_name = "planner_index",
            input_module = "planner_pvt_switch",
            input_message_name = "index",
        },
        con_prepare = {
            output_module = "commander1",
            output_message_name = "mit_motor_command",
            input_module = "planner_pvt_switch",
            input_message_name = "in_1",
        },
        con_damping = {
            output_module = "damping_mode",
            output_message_name = "mit_motor_command",
            input_module = "planner_pvt_switch",
            input_message_name = "in_3",
        },
        -- con_custom = {
        --     output_module = "debugging_mode",
        --     output_message_name = "mit_motor_command",
        --     input_module = "planner_pvt_switch",
        --     input_message_name = "in_4",
        -- },
        con_stance = {
            output_module = "parallel_mech_input_stance",
            output_message_name = "motor_pvt_commands_parallel",
            input_module = "planner_pvt_switch",
            input_message_name = "in_5",
        },
        con_rl_locomotion = {
            output_module = "rl_locomotion",
            output_message_name = "motor_command",
            input_module = "planner_pvt_switch",
            input_message_name = "in_6",
        },
        con_custom_traj = {
            output_module = "parallel_mech_input_custom_traj",
            output_message_name = "motor_pvt_commands_parallel",
            input_module = "planner_pvt_switch",
            input_message_name = "in_7",
        },
        con_rl_locomotion_run = {
            output_module = "rl_locomotion_run",
            output_message_name = "motor_command",
            input_module = "planner_pvt_switch",
            input_message_name = "in_8",
        },
        -- con_rl_fall_recovery = {
        --     output_module = "rl_fall_recovery",
        --     output_message_name = "motor_pvt_commands_parallel",
        --     input_module = "planner_pvt_switch",
        --     input_message_name = "in_9",
        -- },
        -- con_rl_locomotion_gait_2 = {
        --     output_module = "rl_locomotion_gait_2",
        --     output_message_name = "motor_pvt_commands_parallel",
        --     input_module = "planner_pvt_switch",
        --     input_message_name = "in_10",
        -- },

        -------------------------- connect to common_legged_ekf --------------------------
        con_hekf_1 = {
            output_module = "simulator_out",
            output_message_name = "imu_sensor_data",
            input_module = "common_legged_ekf",
            input_message_name = "imu_sensor_data",
        },
        con_hekf_2 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "common_legged_ekf",
            input_message_name = "motor_states",
        },
        con_hekf_4 = {
            output_module = "simulator_out",
            output_message_name = "joint_forces",
            input_module = "common_legged_ekf",
            input_message_name = "motor_forces",
        },
        con_portal_collect_module_2 = {
            output_module = "contact_probability_portal_publish",
            output_message_name = "output",
            input_module = "common_legged_ekf",
            input_message_name = "feet_contact_ref_data",
        },

        -------------------------- connect to legged_estimate_convert --------------------------
        con_common_legged_ekf_convert_1 = {
            output_module = "common_legged_ekf",
            output_message_name = "floating_base_state",
            input_module = "legged_estimate_convert",
            input_message_name = "floating_base",
        },
        con_common_legged_ekf_convert_2 = {
            output_module = "common_legged_ekf",
            output_message_name = "feet_contact_states",
            input_module = "legged_estimate_convert",
            input_message_name = "feet_contact_states_in",
        },
        con_common_legged_ekf_convert_3 = {
            output_module = "common_legged_ekf",
            output_message_name = "rigid_body_traj_states",
            input_module = "legged_estimate_convert",
            input_message_name = "rigid_body_traj_states_in",
        },

        -------------------------- connect to contact_probability_portal_collect --------------------------
        con_portal_collect_module_1 = {
            output_module = "stance_planner",
            output_message_name = "feet_contact_probability_ref_",
            input_module = "contact_probability_portal_collect",
            input_message_name = "input",
        },
        -- con_delay_collect_module_b = {
        --     output_module = "dcm_planner",
        --     output_message_name = "current_walker_mode_index_",
        --     input_module = "dcm_mode_portal_collect",
        --     input_message_name = "input",
        -- },
        -------------------------- connect to contact_probability_portal_publish --------------------------
        -- None
        -------------------------- connect to hand_direct_command_portal_collect --------------------------
        con_hand_collect_module_b = {
            output_module = "hand_direct_command_planner",
            output_message_name = "hand_task_failed_id_",
            input_module = "hand_direct_command_portal_collect",
            input_message_name = "input",
        },

        -------------------------- connect to hand_planner --------------------------
        con_hand_planner_1 = {
            output_module = "legged_estimate_convert",
            output_message_name = "torso_state",
            input_module = "hand_planner",
            input_message_name = "torso_traj_fb_",
        },
        con_hand_planner_2 = {
            output_module = "legged_estimate_convert",
            output_message_name = "hands_state",
            input_module = "hand_planner",
            input_message_name = "hands_traj_fb_",
        },
        con_hand_planner_3 = {
            output_module = "command_manager",
            output_message_name = "hand_action_index",
            input_module = "hand_planner",
            input_message_name = "hand_action_index",
        },
        con_hand_planner_4 = {
            output_module = "command_manager",
            output_message_name = "hand_action_params",
            input_module = "hand_planner",
            input_message_name = "hand_action_params",
        },
        -------------------------- connect to hand_command_planner --------------------------
        con_hand_direct_command_planner_1 = {
            output_module = "legged_estimate_convert",
            output_message_name = "torso_state",
            input_module = "hand_direct_command_planner",
            input_message_name = "torso_traj_fb_",
        },
        con_hand_direct_command_planner_2 = {
            output_module = "legged_estimate_convert",
            output_message_name = "hands_state",
            input_module = "hand_direct_command_planner",
            input_message_name = "hands_traj_fb_",
        },
        con_hand_direct_command_planner_3 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "hand_direct_command_planner",
            input_message_name = "joints_state_fb_",
        },
        con_hand_direct_command_planner_4 = {
            output_module = "common_legged_ekf",
            output_message_name = "floating_base_state",
            input_module = "hand_direct_command_planner",
            input_message_name = "floating_base_state_fb_",
        },   
        con_hand_direct_command_planner_5 = {
            output_module = "command_manager",
            output_message_name = "hand_end_effector_arc_params_left_",
            input_module = "hand_direct_command_planner",
            input_message_name = "hand_end_effector_arc_params_left_",
        },   
        con_hand_direct_command_planner_6 = {
            output_module = "command_manager",
            output_message_name = "hand_end_effector_arc_params_right_",
            input_module = "hand_direct_command_planner",
            input_message_name = "hand_end_effector_arc_params_right_",
        },
        con_hand_direct_command_planner_7 = {
            output_module = "command_manager",
            output_message_name = "hand_end_effector_line_params_left_",
            input_module = "hand_direct_command_planner",
            input_message_name = "hand_end_effector_line_params_left_",
        },  
        con_hand_direct_command_planner_8 = {
            output_module = "command_manager",
            output_message_name = "hand_end_effector_line_params_right_",
            input_module = "hand_direct_command_planner",
            input_message_name = "hand_end_effector_line_params_right_",
        },  
        con_hand_direct_command_planner_9 = {
            output_module = "command_manager",
            output_message_name = "is_hand_intercepted_",
            input_module = "hand_direct_command_planner",
            input_message_name = "is_hand_intercepted_",
        },
        -------------------------- connect to stance_planner --------------------------
        con_stance_planner_1 = {
            output_module = "common_legged_ekf",
            output_message_name = "com_state",
            input_module = "stance_planner",
            input_message_name = "com_traj_fb_",
        },
        con_stance_planner_2 = {
            output_module = "legged_estimate_convert",
            output_message_name = "torso_state",
            input_module = "stance_planner",
            input_message_name = "torso_traj_fb_",
        },
        con_stance_planner_3 = {
            output_module = "legged_estimate_convert",
            output_message_name = "feet_state",
            input_module = "stance_planner",
            input_message_name = "feet_traj_fb_",
        },
        con_stance_planner_4 = {
            output_module = "legged_estimate_convert",
            output_message_name = "feet_contact_states",
            input_module = "stance_planner",
            input_message_name = "feet_td_fb_",
        },
        con_stance_planner_5 = {
            output_module = "legged_estimate_convert",
            output_message_name = "hands_state",
            input_module = "stance_planner",
            input_message_name = "hands_traj_fb_",
        },
        con_stance_planner_6 = {
            output_module = "command_manager",
            output_message_name = "state_command",
            input_module = "stance_planner",
            input_message_name = "torso_cmd_",
        },
        con_stance_planner_7 = {
            output_module = "command_manager",
            output_message_name = "stance_action_index",
            input_module = "stance_planner",
            input_message_name = "action_index_",
        },

        -------------------------- connect to stance_planner_wbc_convert --------------------------
        con_stance_plancvt_1 = {
            output_module = "stance_planner",
            output_message_name = "torso_traj_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "torso_traj_ref_",
        },
        con_stance_plancvt_2 = {
            output_module = "hand_planner",
            output_message_name = "hands_traj_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "hands_traj_ref_",
        },
        con_stance_plancvt_3 = {
            output_module = "stance_planner",
            output_message_name = "feet_traj_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "feet_traj_ref_",
        },
        con_stance_plancvt_4 = {
            output_module = "stance_planner",
            output_message_name = "feet_td_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "feet_td_ref_",
        },
        con_stance_plancvt_5 = {
            output_module = "stance_planner",
            output_message_name = "feet_wrench_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "feet_wrench_ref_",
        },
        con_stance_plancvt_6 = {
            output_module = "stance_planner",
            output_message_name = "com_traj_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "com_traj_ref_",
        },
        con_stance_plancvt_7 = {
            output_module = "legged_estimate_convert",
            output_message_name = "torso_state",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "torso_traj_fb_",
        },
        con_stance_plancvt_8 = {
            output_module = "stance_planner",
            output_message_name = "waist_joint_state_ref_",
            input_module = "stance_planner_wbc_convert",
            input_message_name = "waist_joint_state_ref_",
        },

        -------------------------- connect to stance_wbc_common --------------------------
        con_stance_wbc_common_1 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "stance_wbc_common",
            input_message_name = "joints_state_fb_",
        },
        con_stance_wbc_common_2 = {
            output_module = "common_legged_ekf",
            output_message_name = "floating_base_state",
            input_module = "stance_wbc_common",
            input_message_name = "floating_base_state_fb_",
        },
        con_stance_wbc_common_3 = {
            output_module = "common_legged_ekf",
            output_message_name = "rigid_body_traj_states",
            input_module = "stance_wbc_common",
            input_message_name = "rigid_body_traj_fb_",
        },
        con_stance_wbc_common_5 = {
            output_module = "common_legged_ekf",
            output_message_name = "com_state",
            input_module = "stance_wbc_common",
            input_message_name = "com_traj_fb_",
        },
        con_stance_wbc_common_6 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "floating_base_state_ref_",
            input_module = "stance_wbc_common",
            input_message_name = "floating_base_state_ref_",
        },
        con_stance_wbc_common_8 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "rigid_body_td_ref_",
            input_module = "stance_wbc_common",
            input_message_name = "rigid_body_td_ref_",
        },
        con_stance_wbc_common_9 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "rigid_body_wrenches_ref_",
            input_module = "stance_wbc_common",
            input_message_name = "rigid_body_wrenches_ref_",
        },
        con_stance_wbc_common_10 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "com_state_ref_",
            input_module = "stance_wbc_common",
            input_message_name = "com_traj_ref_",
        },
        -- con_stance_wbc_common_11 = {
        --     output_module = "parallel_mech_output",
        --     output_message_name = "jacobian_inv",
        --     input_module = "stance_wbc_common",
        --     input_message_name = "jacobian_ps_inv_fb_",
        -- },
        con_stance_wbc_common_12 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "rigid_body_traj_ref_",
            input_module = "stance_wbc_common",
            input_message_name = "rigid_body_traj_ref_",
        },
        con_stance_wbc_common_13 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "joints_state_ref_",
            input_module = "stance_wbc_common",
            input_message_name = "joints_state_ref_",
        },

        -------------------------- connect to stance_pvt --------------------------
        con_stance_pvt_1 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "stance_pvt",
            input_message_name = "joints_state_fb_",
        },
        con_stance_pvt_2 = {
            output_module = "stance_wbc_common",
            output_message_name = "joint_tor_tar_",
            input_module = "stance_pvt",
            input_message_name = "joint_tor_tar_",
        },
        con_stance_pvt_3 = {
            output_module = "stance_wbc_common",
            output_message_name = "joint_acc_serial_",
            input_module = "stance_pvt",
            input_message_name = "joint_acc_tar_",
        }, 
        con_stance_pvt_4 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "rigid_body_td_ref_",
            input_module = "stance_pvt",
            input_message_name = "rigid_body_td_ref_",
        },
        con_stance_pvt_5 = {
            output_module = "common_legged_ekf",
            output_message_name = "floating_base_state",
            input_module = "stance_pvt",
            input_message_name = "floating_base_state_fb_",
        },
        con_stance_pvt_6 = {
            output_module = "stance_wbc_common",
            output_message_name = "error_and_give_up_",
            input_module = "stance_pvt",
            input_message_name = "error_and_give_up_",
        },
        con_stance_pvt_7 = {   
            output_module = "stance_planner_wbc_convert",
            output_message_name = "joints_state_ref_",
            input_module = "stance_pvt",
            input_message_name = "joints_state_ref_",
        },
        con_stance_pvt_8 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "com_state_ref_",
            input_module = "stance_pvt",
            input_message_name = "com_traj_ref_",
        },
        con_stance_pvt_9 = {
            output_module = "common_legged_ekf",
            output_message_name = "com_state",
            input_module = "stance_pvt",
            input_message_name = "com_traj_fb_",
        },
        con_stance_pvt_10 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "rigid_body_traj_ref_",
            input_module = "stance_pvt",
            input_message_name = "rigid_body_traj_ref_",
        },
        con_stance_pvt_11 = {
            output_module = "common_legged_ekf",
            output_message_name = "rigid_body_traj_states",
            input_module = "stance_pvt",
            input_message_name = "rigid_body_traj_fb_",
        },
        con_stance_pvt_12 = {
            output_module = "stance_planner_wbc_convert",
            output_message_name = "floating_base_state_ref_",
            input_module = "stance_pvt",
            input_message_name = "floating_base_state_ref_",
        },

        -------------------------- connect to stance_mode_portal_collect --------------------------
        con_portal_collect_module_b = {
            output_module = "stance_planner",
            output_message_name = "current_stance_mode_index_",
            input_module = "stance_mode_portal_collect",
            input_message_name = "input",
        },

        -------------------------- connect to stance_mode_portal_publish --------------------------
        -- None

        -------------------------- connect to publisher --------------------------
        con_jmo_pub_1 = {
            output_module = "joint_map_output",
            output_message_name = "joint_states_mapped",
            input_module = "publisher",
            input_message_name = "joint_states_parallel",
        },
        con_jmo_pub_2 = {
            output_module = "joint_map_output",
            output_message_name = "joint_forces_mapped",
            input_module = "publisher",
            input_message_name = "joint_forces_parallel",
        },
        con_hekf_pub_1 = {
            output_module = "common_legged_ekf",
            output_message_name = "rigid_body_traj_states",
            input_module = "publisher",
            input_message_name = "rigid_body_traj_fb",
        },

        con_hekf_pub_2 = {
            output_module = "common_legged_ekf",
            output_message_name = "floating_base_state",
            input_module = "publisher",
            input_message_name = "floating_base_state_from_ekf",
        },
        con_publisher_2 = {
            output_module = "rl_locomotion_run",
            output_message_name = "odom",
            input_module = "publisher",
            input_message_name = "current_odometer_est",
        },
        con_publisher_3 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "publisher",
            input_message_name = "joint_states_serial",
        },
        con_publisher_4 = {
            output_module = "simulator_out",
            output_message_name = "joint_forces",
            input_module = "publisher",
            input_message_name = "joint_forces_serial",
        },
        -- con_publisher_5 = {
        --     output_module = "rl_locomotion_gait_2",
        --     output_message_name = "odom",
        --     input_module = "publisher",
        --     input_message_name = "current_odometer_est_gait_2",
        -- },
        con_publisher_imu_state = {
            output_module = "simulator_out",
            output_message_name = "imu_sensor_data",
            input_module = "publisher",
            input_message_name = "imu_sensor_data",
        },
        con_publisher_motor_temp = {                            -- ====== only in realbot ======
            output_module = "simulator_out",
            output_message_name = "motor_temp_group",
            input_module = "publisher",
            input_message_name = "motor_temp",
        },
        -- con_publisher_fall_down_recovery_state = {
        --     output_module = "fall_down_recovery",
        --     output_message_name = "fall_down_recovery_state",
        --     input_module = "publisher",
        --     input_message_name = "fall_down_recovery_state",
        -- },
        -- con_publisher_is_recovery_avalible = {
        --     output_module = "fall_down_recovery",
        --     output_message_name = "is_recovery_avalible",
        --     input_module = "publisher",
        --     input_message_name = "is_recovery_avalible",
        -- },
        -- con_publisher_current_planner_idx = {
        --     output_module = "command_manager",
        --     output_message_name = "planner_index",
        --     input_module = "publisher",
        --     input_message_name = "current_planner_index",
        -- },

        -------------------------- connect to rviz --------------------------
        con_cle_rviz = {
            output_module = "common_legged_ekf",
            output_message_name = "floating_base_state",
            input_module = "rviz",
            input_message_name = "floating_base_state",
        },
        con_cle_rviz_2 = {
            output_module = "common_legged_ekf",
            output_message_name = "com_state",
            input_module = "rviz",
            input_message_name = "com_state",
        },
        con_pmo_rviz = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "rviz",
            input_message_name = "joint_states_serial",
        },
        con_so_rviz = {                                         -- ====== only in realbot ======
            output_module = "simulator_out",
            output_message_name = "motor_temp_group",
            input_module = "rviz",
            input_message_name = "motor_temp",
        },

        -------------------------- connect to intercept_motor_cmd --------------------------
        con7 = {
            output_module = "planner_pvt_switch",
            output_message_name = "out",
            input_module = "intercept_motor_cmd",
            input_message_name = "motor_pvt_",
        },
        con_commander_manager_2_intercept_motor_cmd = {
            output_module = "command_manager",
            output_message_name = "user_motor_pvt_cmd_",
            input_module = "intercept_motor_cmd",
            input_message_name = "user_motor_pvt_cmd_",
        },
        con_commander_manager_2_intercept_planner_idx = {
            output_module = "command_manager",
            output_message_name = "planner_index",
            input_module = "intercept_motor_cmd",
            input_message_name = "current_planner_index_",
        },
        con_hand_command_2_intercept_motor_cmd = {
            output_module = "hand_direct_command_planner",
            output_message_name = "motor_hand_command_",
            input_module = "intercept_motor_cmd",
            input_message_name = "hand_command_motor_pvt_cmd_",
        },
        ------------------------- connect to rl_locomotion ---------------------------------
        con_rl_locomotion_1 = {
            output_module = "command_manager",
            output_message_name = "state_command",
            input_module = "rl_locomotion",
            input_message_name = "vel_cmd",
        },
        con_rl_locomotion_2 = {
            output_module = "command_manager",
            output_message_name = "gait_index",
            input_module = "rl_locomotion",
            input_message_name = "wave_hand_index",
        },
        con_rl_locomotion_3 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "rl_locomotion",
            input_message_name = "motor_state_feedback",
        },
        con_rl_locomotion_4 = {
            output_module = "simulator_out",
            output_message_name = "imu_sensor_data",
            input_module = "rl_locomotion",
            input_message_name = "imu_feedback",
        },
        -- con_rl_locomotion_5 = {
        --     output_module = "parallel_mech_output",
        --     output_message_name = "jacobian_inv",
        --     input_module = "rl_locomotion",
        --     input_message_name = "jacobian_inv",
        -- },

        ------------------------- connect to rl_locomotion_run ---------------------------------
        con_rl_locomotion_run_1 = {
            output_module = "command_manager",
            output_message_name = "state_command",
            input_module = "rl_locomotion_run",
            input_message_name = "vel_cmd",
        },
        con_rl_locomotion_run_2 = {
            output_module = "command_manager",
            output_message_name = "gait_index",
            input_module = "rl_locomotion_run",
            input_message_name = "wave_hand_index",
        },
        con_rl_locomotion_run_3 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "rl_locomotion_run",
            input_message_name = "motor_state_feedback",
        },
        con_rl_locomotion_run_4 = {
            output_module = "simulator_out",
            output_message_name = "imu_sensor_data",
            input_module = "rl_locomotion_run",
            input_message_name = "imu_feedback",
        },
        -- con_rl_locomotion_run_5 = {
        --     output_module = "parallel_mech_output",
        --     output_message_name = "jacobian_inv",
        --     input_module = "rl_locomotion_run",
        --     input_message_name = "jacobian_inv",
        -- },
        con_rl_locomotion_run_6 = {
            output_module = "command_manager",
            output_message_name = "gait_freq_indicator",
            input_module = "rl_locomotion_run",
            input_message_name = "high_freq_index",
        },
        con_rl_locomotion_run_7 = {
            output_module = "command_manager",
            output_message_name = "yaw_tracking_indicator",
            input_module = "rl_locomotion_run",
            input_message_name = "yaw_tracking_index",
        },
        ------------------------- connect to rl_locomotion_gait_2 ---------------------------------
        -- con_rl_locomotion_gait_2_1 = {
        --     output_module = "command_manager",
        --     output_message_name = "state_command",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "vel_cmd",
        -- },
        -- con_rl_locomotion_gait_2_2 = {
        --     output_module = "command_manager",
        --     output_message_name = "gait_index",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "wave_hand_index",
        -- },
        -- con_rl_locomotion_gait_2_3 = {
        --     output_module = "parallel_mech_output",
        --     output_message_name = "joint_states_serial",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "motor_state_feedback",
        -- },
        -- con_rl_locomotion_gait_2_4 = {
        --     output_module = "simulator_out",
        --     output_message_name = "imu_sensor_data",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "imu_feedback",
        -- },
        -- con_rl_locomotion_gait_2_5 = {
        --     output_module = "parallel_mech_output",
        --     output_message_name = "jacobian_inv",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "jacobian_inv",
        -- },
        -- con_rl_locomotion_gait_2_6 = {
        --     output_module = "command_manager",
        --     output_message_name = "gait_freq_indicator",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "high_freq_index",
        -- },
        -- con_rl_locomotion_gait_2_7 = {
        --     output_module = "command_manager",
        --     output_message_name = "yaw_tracking_indicator",
        --     input_module = "rl_locomotion_gait_2",
        --     input_message_name = "yaw_tracking_index",
        -- },

        -------------------------- connect to custom_traj --------------------------
        con_custom_traj_fb = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "custom_traj",
            input_message_name = "motor_state_feedback",
        },
        con_custom_traj_gait_idx = {
            output_module = "command_manager",
            output_message_name = "gait_index",
            input_module = "custom_traj",
            input_message_name = "traj_idx",
        },
    },
}

function graph.init(arg1)
    is_real_bot = arg1
    if is_real_bot then
        graph.modules.simulator_out = "rs_sdk_interface::RsSdkOutput"
        graph.modules.simulator_in = "rs_sdk_interface::RsSdkInput"
    else
        -- graph.import.webots_simulator = "lib/libwebots_interface.so"
        graph.import.pid = "lib/libmodule_pid.so"
        -- graph.import.noise = "lib/libmsg_process.so"
	    -- graph.import.drawer_backend = "lib/libutils_drawings_webots_backend.so"

        -------------------------- set modules
        graph.modules.pid = "pid::MITGroupController"
        -- graph.modules.noise = "msg_process::InputNoise"
        graph.modules.simulator_out = "rs_sdk_interface::RsSdkOutput"
        graph.modules.simulator_in = "rs_sdk_interface::RsSdkInput"

        graph.connections.con2 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "commander1",
            input_message_name = "motor_state_feedback",
        }

        graph.connections.con_pmo_dpm = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "damping_mode",
            input_message_name = "motor_state_feedback",
        }
    
        -------------------------- connect to parallel_mech_input_stance --------------------------
        graph.connections.con_pi_1_stance = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "parallel_mech_input_stance",
            input_message_name = "joint_states_parallel",
        }

        -------------------------- connect to parallel_mech_input_custom_traj --------------------------
        graph.connections.con_pi_1_custom_traj = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "parallel_mech_input_custom_traj",
            input_message_name = "joint_states_parallel",
        }

           -------------------------- connect to parallel_mech_input_custom_traj --------------------------
        -- graph.connections.con_pi_1_custom_mode = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_states",
        --     input_module = "parallel_mech_input_custom_mode",
        --     input_message_name = "joint_states_parallel",
        -- }

        -------------------------- connect to parallel_mech_input_rl --------------------------
        -- graph.connections.con_pi_1_rl = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_states",
        --     input_module = "parallel_mech_input_rl",
        --     input_message_name = "joint_states_parallel",
        -- }

        -------------------------- connect to parallel_mech_output --------------------------
        graph.connections.con_noise_po_1 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "parallel_mech_output",
            input_message_name = "joint_states_parallel",
        }
        graph.connections.con_noise_po_2 = {
            output_module = "simulator_out",
            output_message_name = "joint_forces",
            input_module = "parallel_mech_output",
            input_message_name = "joint_forces_parallel",
        }

        -------------------------- connect to intercept_motor_cmd --------------------------
        graph.connections.pr_intercept_motor_cmd_1 = {
            output_module = "planner_pvt_switch",
            output_message_name = "out",
            input_module = "intercept_motor_cmd",
            input_message_name = "motor_pvt_",
        }

        -------------------------- connect to pid --------------------------
        graph.connections.intercept_motor_cmd_pid_1 = {
            output_module = "intercept_motor_cmd",
            output_message_name = "motor_pvt_intercepted_",
            input_module = "pid",
            input_message_name = "motor_commands",
        }
        graph.connections.sim_out_pid_1 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "pid",
            input_message_name = "motor_states",
        }

        -------------------------- connect to simulator_in --------------------------
        graph.connections.pid_sim_in_1 = {
            output_module = "pid",
            output_message_name = "force_commands",
            input_module = "simulator_in",
            input_message_name = "motor_force_commands",
        }

        -------------------------- connect to noise --------------------------
        -- graph.connections.con_ni_1 = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_states",
        --     input_module = "noise",
        --     input_message_name = "joint_states",
        -- }
        -- graph.connections.con_ni_2 = {
        --     output_module = "simulator_out",
        --     output_message_name = "imu_sensor_data",
        --     input_module = "noise",
        --     input_message_name = "imu_sensor_data",
        -- }
        -- graph.connections.con_ni_3 = {
        --     output_module = "simulator_out",
        --     output_message_name = "torso_and_feet_pos",
        --     input_module = "noise",
        --     input_message_name = "torso_and_feet_pos",
        -- }
        -- graph.connections.con_ni_4 = {
        --     output_module = "simulator_out",
        --     output_message_name = "joint_forces",
        --     input_module = "noise",
        --     input_message_name = "joint_forces",
        -- }

        -------------------------- connect to publisher --------------------------
        graph.connections.con_noise_pub_1 = {
            output_module = "simulator_out",
            output_message_name = "joint_states",
            input_module = "publisher",
            input_message_name = "joint_states_parallel",
        }
        graph.connections.con_noise_pub_2 = {
            output_module = "simulator_out",
            output_message_name = "joint_forces",
            input_module = "publisher",
            input_message_name = "joint_forces_parallel",
        }

        -------------------------- connect to common_legged_ekf --------------------------
        graph.connections.con_hekf_1 = {
            output_module = "simulator_out",
            output_message_name = "imu_sensor_data",
            input_module = "common_legged_ekf",
            input_message_name = "imu_sensor_data",
        }

        -------------------------- cancel connection
        graph.connections.con_jmo_1 = nil
        graph.connections.con_jmo_2 = nil
        graph.connections.con_jmo_pub_1 = nil
        graph.connections.con_jmo_pub_2 = nil
        graph.connections.con_jmi_1 = nil
        graph.connections.con_po_1 = nil
        graph.connections.con_po_2 = nil
        graph.connections.con7 = nil
        graph.connections.con_so_rviz = nil
        graph.connections.con_intercept_motor_cmd_2_joint_map_input = nil
        graph.connections.con_publisher_motor_temp = nil

        -------------------------- cancel modules
        graph.modules.joint_map_output = nil
        graph.modules.joint_map_input = nil
    end
end

return graph
