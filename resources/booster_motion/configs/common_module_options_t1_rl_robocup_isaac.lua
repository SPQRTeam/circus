package.path = package.path .. ";../src/tests/?.lua"

controller_base_dt_ms = 8

kp_test = {
    -- on ground
    -- 5., 5.,
    -- 40., 50., 20., 10.,
    -- 40., 50., 20., 10.,
    -- 100., 
    -- 350., 350., 180., 350., 550., 550.,
    -- 350., 350., 180., 350., 550., 550.,

    -- on air
    5., 5.,
    20., 20., 10., 5.,
    20., 20., 10., 5.,
    20., 
    120., 70., 40., 120., 150., 80.,
    120., 70., 40., 120., 150., 80.,

    -- 0., 0.,
    -- 0., 0., 0., 0.,
    -- 0., 0., 0., 0.,
    -- 0., 
    -- 0., 0., 0., 0., 0., 0.,
    -- 0., 0., 0., 0., 0., 0.,
}

kd_test = {
    -- on ground
    -- .1, .1,
    -- .5, 1.5, .2, .2,
    -- .5, 1.5, .2, .2,
    -- 5.0,
    -- 7.5, 7.5, 3., 5.5, 7.5, 7.5,
    -- 7.5, 7.5, 3., 5.5, 7.5, 7.5,

    -- on air
    .1, .1,
    .2, .2, .1, .1,
    .2, .2, .1, .1,
    .2,
    .4, .2, .2, .4, .35, .1,
    .4, .2, .2, .4, .35, .1,

    -- 0., 0.,
    -- 0., 0., 0., 0.,
    -- 0., 0., 0., 0.,
    -- 0., 
    -- 0., 0., 0., 0., 0., 0.,
    -- 0., 0., 0., 0., 0., 0.,
}

ready_pos_test = {
    0.00,  0.00,
    0.60, -1.40,  0.00, -1.50,
    0.60,  1.40,  0.00,  1.50,
    0.0,
    -0.38, -0.0,  0.0,  0.8,  -0.43,  0.0,
    -0.38,  0.0, -0.0,  0.8,  -0.43,  0.0,

    -- -0.4, -0.0,  0.0,  0.8, 0.299,  0.274,
    -- -0.4,  0.0, -0.0,  0.8, 0.299,  0.274,

    -- 0.0, 0.0,
    -- 0.6, -1.4, 0.0, -1.5,
    -- 0.6,  1.4, 0.0,  1.5,
    -- 0.0,
    -- -0.4, -0.03,  0.05,  0.7, -0.35,  0.03,
    -- -0.4,  0.03, -0.05,  0.7, -0.35, -0.03,

    -- 0., 0.,
    -- 0., 0., 0., 0.,
    -- 0., 0., 0., 0.,
    -- 0., 
    -- 0., 0.0, 0.0, 0., 0., 0.0,
    -- 0., 0.0, 0.0, 0., 0., 0.0,
}

local options = {
    simulator_out = {
        is_real_bot = false,
        is_imu_rotated_ = false,
        record_data = true,

        proto_index_imu_data_ = 3,
        proto_index_joint_num_ = 7,
        proto_index_joint_val_ = 6,
        proto_index_remote_data_ = 4,
        num_motor_ = 23,        
        
        max_lin_vel_x_ = 0.25,
        max_lin_vel_y_ = 0.07,
        max_rot_vel_z_ = 1.3,
    },

    simulator_in = {
        record_data = true,

        step_len_ = controller_base_dt_ms, 
        proto_index_joint_cmd_ = 5,
        use_pvt_ = false,
        num_motor_ = 23,
        motor_max_torque_value_ = {
            7.0, 7.0,
            36.0, 36.0, 36.0, 36.0,
            36.0, 36.0, 36.0, 36.0,
            60.0,
            90.0, 60.0, 60.0, 130.0, 36.0, 50.0,
            90.0, 60.0, 60.0, 130.0, 36.0, 50.0,
        },
        motor_max_position_value_ = {
            3.14159, 3.14159, 
            3.14159, 3.14159, 3.14159, 3.14159, 
            3.14159, 3.14159, 3.14159, 3.14159, 
            3.14159, 
            3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159,
            3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159,
        },
    },

    command_manager = {
        gait_list = {
            "stand",
            "walk",
        },
        vel_x_max = 0.4,
        vel_y_max = 0.2,
        rotvel_max = 1.0,

        roll_max = 0.1,
        pitch_max = 0.1,
        height_bias_max = 0.02,
        height = 0.48,

        -- desc: 当连接多个 joystick 时，表示使用哪个 joystick
        -- scope: 
        -- unit: m
        joystick_index = 0,

        use_joystick = true,
        -- lcm_channel = "CHANNEL_DECISION_1",

        default_planner_index = 1,

        motor_cmd_diff_max = 0.4,
        low_cmd_kp_from_high_api = 5.0,
        low_cmd_kd_from_high_api = 0.5,
        head_yaw_cmd_speed = 0.005,
        head_pitch_cmd_speed = 0.005,
        head_pitch_index = 1,
        head_yaw_index = 0,
        q_min = {
            -1.5708, -0.3491,
            -3.3161, -1.7453, -2.2689, -2.4435,
            -3.3161, -1.5708, -2.2689, -0.0000,
            -3.1416,
            -3.1416, -0.5236, -1.0472, -0.0000, -1.8727, -1.4363,
            -3.1416, -1.5708, -1.0472, -0.0000, -1.8727, -1.4363,
        },
        q_max = {
            1.5708,  1.2217,
            1.2217,  1.5708,  2.2689,  0.0000,
            1.2217,  1.7453,  2.2689,  2.4435,
            3.1416,
            3.1416,  1.5708,  1.0472,  2.3387,  1.3491,  1.4363,
            3.1416,  0.5236,  1.0472,  2.3387,  1.3491,  1.4363,
        },
    },

    publisher = {
        send_over_temp_light_status_threshold = 85.0,

        head_name = "H2",
        head_point = {0.0613, 0.0, 0.0908},

        left_foot_name = "left_foot_link",
        right_foot_name = "right_foot_link",
        feet_point_pos_local = {0.015,  0.000, -0.030,}
    },
    
    joint_map_output = {
        record_data = true,
        n_joint_original = 23,
        n_joint_mapped = 23,
        mapping = {
            0, 1, 
            2, 3, 4, 5, 
            6, 7, 8, 9, 
            10, 
            11, 12, 13, 14, 15, 16,
            17, 18, 19, 20, 21, 22,
        },
        pos_factor = {
            1., 1.,
            1., 1., 1., 1.,
            1., 1., 1., 1.,
            1., 
            1., 1., 1., 1., 1., 1.,
            1., 1., 1., 1., 1., 1.,
        },
        torq_factor = {
            1., 1.,
            1., 1., 1., 1.,
            1., 1., 1., 1.,
            1., 
            1., 1., 1., 1., 1., 1.,
            1., 1., 1., 1., 1., 1.,
        },
    },

    joint_map_input = {
        record_data = true,
        use_pvt_ = true,
        n_joint_original = 23,
        n_joint_mapped = 23,
        mapping = {
            0, 1, 
            2, 3, 4, 5, 
            6, 7, 8, 9, 
            10, 
            11, 12, 13, 14, 15, 16,
            17, 18, 19, 20, 21, 22,
        },
        pos_factor = {
            1., 1.,
            1., 1., 1., 1.,
            1., 1., 1., 1.,
            1., 
            1., 1., 1., 1., 1., 1.,
            1., 1., 1., 1., 1., 1.,
        },
        torq_factor = {
            1., 1.,
            1., 1., 1., 1.,
            1., 1., 1., 1.,
            1., 
            1., 1., 1., 1., 1., 1.,
            1., 1., 1., 1., 1., 1.,
        },
        kp_factor = {
            1., 1.,
            1., 1., 1., 1.,
            1., 1., 1., 1.,
            1., 
            1., 1., 1., 1., 1., 1.,
            1., 1., 1., 1., 1., 1.,
        },
        kd_factor = {
            1., 1.,
            1., 1., 1., 1.,
            1., 1., 1., 1.,
            1., 
            1., 1., 1., 1., 1., 1.,
            1., 1., 1., 1., 1., 1.,
        },
    },

    debugging_mode = {
        n_joint = 23,
    },
    damping_mode = {
        record_data = true,
        n_joint = 23,
        kd = {
            5., 5.,
            5., 5., 5., 5.,
            5., 5., 5., 5.,
            5.,
            50., 50., 50., 50., 5., 5.,
            50., 50., 50., 50., 5., 5.,
        }
    },

    parallel_mech_input = {
        record_data = true,
        use_pvt_ = true,
        joint_idx_parallel_ = {15, 16, 21, 22},
        joint_idx_serial_ = {15, 16, 21, 22},
        ik_clockwise_ = false,
        fk_same_dir_ = false,
        dist_joint_LR_ = 0.06,
        len_link_L_ = 0.18,
        len_link_R_ = 0.12,
        r_joint_ = 0.042,
        pos_ankle_in_limb_ = {0.004, 0., -0.184},
        pos_ankle_shift_ = {0., 0., -0.012},
        pos_heel_in_foot_ = {-0.034, 0., 0.0215},
        width_heel_ = 0.043,
        mech_pitch_offset_deg_ = -109.0056,
        mech_roll_offset_deg_ = 10.1127,
        joint_parallel_zero_pos_ = {1.7982, 1.9747, 1.7982, 1.9747,},
        joint_serial_zero_pos_ = {0.016057, 0., 0.016057, 0.,},
        is_mirror_ = {true, false},
    },

    parallel_mech_input_stance = {
        record_data = true,
        use_pvt_ = true,
        joint_idx_parallel_ = {15, 16, 21, 22},
        joint_idx_serial_ = {15, 16, 21, 22},
        ik_clockwise_ = false,
        fk_same_dir_ = false,
        dist_joint_LR_ = 0.06,
        len_link_L_ = 0.18,
        len_link_R_ = 0.12,
        r_joint_ = 0.042,
        pos_ankle_in_limb_ = {0.004, 0., -0.184},
        pos_ankle_shift_ = {0., 0., -0.012},
        pos_heel_in_foot_ = {-0.034, 0., 0.0215},
        width_heel_ = 0.043,
        mech_pitch_offset_deg_ = -109.0056,
        mech_roll_offset_deg_ = 10.1127,
        joint_parallel_zero_pos_ = {1.7982, 1.9747, 1.7982, 1.9747,},
        joint_serial_zero_pos_ = {0.016057, 0., 0.016057, 0.,},
        is_mirror_ = {true, false},
    },

    parallel_mech_input_rl = {
        record_data = true,
        use_pvt_ = true,
        joint_idx_parallel_ = {15, 16, 21, 22},
        joint_idx_serial_ = {15, 16, 21, 22},
        ik_clockwise_ = false,
        fk_same_dir_ = false,
        dist_joint_LR_ = 0.06,
        len_link_L_ = 0.18,
        len_link_R_ = 0.12,
        r_joint_ = 0.042,
        pos_ankle_in_limb_ = {0.004, 0., -0.184},
        pos_ankle_shift_ = {0., 0., -0.012},
        pos_heel_in_foot_ = {-0.034, 0., 0.0215},
        width_heel_ = 0.043,
        mech_pitch_offset_deg_ = -109.0056,
        mech_roll_offset_deg_ = 10.1127,
        joint_parallel_zero_pos_ = {1.7982, 1.9747, 1.7982, 1.9747,},
        joint_serial_zero_pos_ = {0.016057, 0., 0.016057, 0.,},
        is_mirror_ = {true, false},
    },

    parallel_mech_input_custom_traj = {
        record_data = true,
        use_pvt_ = true,
        joint_idx_parallel_ = {15, 16, 21, 22},
        joint_idx_serial_ = {15, 16, 21, 22},
        ik_clockwise_ = false,
        fk_same_dir_ = false,
        dist_joint_LR_ = 0.06,
        len_link_L_ = 0.18,
        len_link_R_ = 0.12,
        r_joint_ = 0.042,
        pos_ankle_in_limb_ = {0.004, 0., -0.184},
        pos_ankle_shift_ = {0., 0., -0.012},
        pos_heel_in_foot_ = {-0.034, 0., 0.0215},
        width_heel_ = 0.043,
        mech_pitch_offset_deg_ = -109.0056,
        mech_roll_offset_deg_ = 10.1127,
        joint_parallel_zero_pos_ = {1.7982, 1.9747, 1.7982, 1.9747,},
        joint_serial_zero_pos_ = {0.016057, 0., 0.016057, 0.,},
        is_mirror_ = {true, false},
    },

    parallel_mech_input_custom_mode = {
        record_data = true,
        use_pvt_ = true,
        joint_idx_parallel_ = {15, 16, 21, 22},
        joint_idx_serial_ = {15, 16, 21, 22},
        ik_clockwise_ = false,
        fk_same_dir_ = false,
        dist_joint_LR_ = 0.06,
        len_link_L_ = 0.18,
        len_link_R_ = 0.12,
        r_joint_ = 0.042,
        pos_ankle_in_limb_ = {0.004, 0., -0.184},
        pos_ankle_shift_ = {0., 0., -0.012},
        pos_heel_in_foot_ = {-0.034, 0., 0.0215},
        width_heel_ = 0.043,
        mech_pitch_offset_deg_ = -109.0056,
        mech_roll_offset_deg_ = 10.1127,
        joint_parallel_zero_pos_ = {1.7982, 1.9747, 1.7982, 1.9747,},
        joint_serial_zero_pos_ = {0.016057, 0., 0.016057, 0.,},
        is_mirror_ = {true, false},
    },

    parallel_mech_output = {
        record_data = true,
        use_fk_hotstart_ = true,
        joint_idx_parallel_ = {15, 16, 21, 22},
        joint_idx_serial_ = {15, 16, 21, 22},
        ik_clockwise_ = false,
        fk_same_dir_ = false,
        dist_joint_LR_ = 0.06,
        len_link_L_ = 0.18,
        len_link_R_ = 0.12,
        r_joint_ = 0.042,
        pos_ankle_in_limb_ = {0.004, 0., -0.184},
        pos_ankle_shift_ = {0., 0., -0.012},
        pos_heel_in_foot_ = {-0.034, 0., 0.0215},
        width_heel_ = 0.043,
        mech_pitch_offset_deg_ = -109.0056,
        mech_roll_offset_deg_ = 10.1127,
        joint_parallel_zero_pos_ = {1.7982, 1.9747, 1.7982, 1.9747,},
        joint_serial_zero_pos_ = {0.016057, 0., 0.016057, 0.,},
        is_mirror_ = {true, false},
    },

    noise = {
        gyro_noise_amp = 0.000,
        lin_vel_noise_amp = 0.000,
        rot_vel_noise_amp = 0.000,
        acc_noise_amp = 0.000,
        q_noise_amp = 0.000,
        dq_noise_amp = 0.00,
        torq_noise_amp = 0.0,
        random_seed = 123,
    },

    contact_probability_portal_collect = {
        portal_pair_key = "portal for contact_probability",
    },
    contact_probability_portal_publish = {
        portal_pair_key = "portal for contact_probability",
    },

    rviz = {
        lcm_channel = "CHANNEL_RVIZ_STATE",
    },

    hand_planner = {
        record_data_ = false,
        
        action_list_ = {"wave_L", "pick_ball", "wave_R",},

        action_wave_L = {
            param_names = {"period", "wy_1", "wy_2"},
            L = {
                enable = true,
                init_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.2, 0.2, 0.0},
                        lin_vel = {0.2, 0.0, 0.1},
                        rpy = {0., 0., 0.},
                        T = 0.5,
                    },
                    WP_2 = {
                        pos = {0.3, 0.25, 0.45},
                        lin_vel = {0., 0., 0.},
                        rpy = {-0.2, -1.3, 0.1},
                        T = 1.,
                    },
                },
                repeat_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.3, 0.1, 0.45},
                        lin_vel = {0., 0., 0.},
                        rpy = {0.5, -1.3, -0.1},
                        T = 0.5,
                    },
                    WP_2 = {
                        pos = {0.3, 0.3, 0.45},
                        lin_vel = {0., 0., 0.},
                        rpy = {-0.5, -1.3, 0.1},
                        T = 0.5,
                    },
                },
                exit_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.2, 0.2, 0.0},
                        lin_vel = {-0.2, 0.0, 0.1},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.1, 0.2, -0.1},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., 0.},
                        T = 0.5,
                    },
                },
            },
            R = {
                enable = false,
            },
        },
        action_wave_R = {
            L = {
                enable = false,
            },
            R = {
                enable = true,
                init_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.2, -0.2, 0.0},
                        lin_vel = {0.2, 0.0, 0.1},
                        rpy = {0., 0., 0.},
                        T = 0.5,
                    },
                    WP_2 = {
                        pos = {0.3, -0.25, 0.45},
                        lin_vel = {0., 0., 0.},
                        rpy = {0.2, -1.3, -0.1},
                        T = 1.,
                    },
                },
                repeat_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.3, -0.1, 0.45},
                        lin_vel = {0., 0., 0.},
                        rpy = {-0.5, -1.3, 0.1},
                        T = 0.5,
                    },
                    WP_2 = {
                        pos = {0.3, -0.3, 0.45},
                        lin_vel = {0., 0., 0.},
                        rpy = {0.5, -1.3, -0.1},
                        T = 0.5,
                    },
                },
                exit_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.2, -0.2, 0.0},
                        lin_vel = {-0.2, 0.0, 0.1},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.1, -0.2, -0.1},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., 0.},
                        T = 0.5,
                    },
                },
            },
        },

        action_pick_ball = {
            param_names = {
                "ready_L_x", "ready_L_y", "ready_L_z", 
                "ready_R_x", "ready_R_y", "ready_R_z",
                "catch_L_x", "catch_L_y", "catch_L_z",
                "catch_R_x", "catch_R_y", "catch_R_z",

            },
            L = {
                enable = true,
                init_seq = {
                    seq_len = 3,
                    WP_1 = {
                        pos = {0.15, 0.3, -0.0},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., -0.4},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.3, 0.25, 0.05},
                        lin_vel = {0.2, 0.0, 0.1},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                    WP_3 = {
                        pos = {0.45, 0.25, 0.2},
                        _pos = {"ready_L_x", "ready_L_y", "ready_L_z"},
                        lin_vel = {0.0, 0.0, 0.0},
                        rpy = {0., -0.03, 0.},
                        T = 1.,
                    },
                },
                repeat_seq = {
                    seq_len = 1,
                    WP_1 = {
                        pos = {0.48, 0.11, 0.18},
                        _pos = {"catch_L_x", "catch_L_y", "catch_L_z"},
                        lin_vel = {0.0, 0.0, 0.0},
                        rpy = {0., -0.15, -0.0},
                        T = 1.,
                    },
                },
                exit_seq = {
                    seq_len = 5,
                    WP_1 = {
                        pos = {0.23, 0.11, 0.5},
                        lin_vel = {0., 0.0, 0.},
                        rpy = {0., -1.45, -0.0},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.23, 0.11, 0.50},
                        lin_vel = {0., 0.0, 0.},
                        rpy = {0., -1.45, -0.0},
                        T = 0.5,
                    },
                    WP_3 = {
                        pos = {0.40, 0.11, 0.45},
                        lin_vel = {-0.2, 0.0, -0.0},
                        rpy = {0., -0.8, -0.0},
                        T = 0.3,
                    },
                    WP_4 = {
                        pos = {0.45, 0.2, 0.20},
                        lin_vel = {0.00, 0.0, -0.2},
                        rpy = {0., -0.3, 0.},
                        T = 1.,
                    },
                    WP_5 = {
                        pos = {0.1, 0.2, -0.1},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                },
            },
            R = {
                enable = true,
                init_seq = {
                    seq_len = 3,
                    WP_1 = {
                        pos = {0.15, -0.3, -0.0},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., 0.4},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.3, -0.25, 0.05},
                        lin_vel = {0.2, 0.0, 0.1},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                    WP_3 = {
                        pos = {0.45, -0.25, 0.2},
                        _pos = {"ready_R_x", "ready_R_y", "ready_R_z"},
                        lin_vel = {0.0, 0.0, 0.0},
                        rpy = {0., -0.03, 0.},
                        T = 1.,
                    },
                },
                repeat_seq = {
                    seq_len = 1,
                    WP_1 = {
                        pos = {0.48, -0.11, 0.18},
                        _pos = {"catch_R_x", "catch_R_y", "catch_R_z"},
                        lin_vel = {0.0, 0.0, 0.0},
                        rpy = {0., -0.15, -0.0},
                        T = 1.,
                    },
                },
                exit_seq = {
                    seq_len = 5,
                    WP_1 = {
                        pos = {0.23, -0.11, 0.5},
                        lin_vel = {0., 0.0, 0.},
                        rpy = {0., -1.45, -0.0},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.23, -0.11, 0.50},
                        lin_vel = {0., 0.0, 0.},
                        rpy = {0., -1.45, -0.0},
                        T = 0.5,
                    },
                    WP_3 = {
                        pos = {0.40, -0.11, 0.45},
                        lin_vel = {-0.2, 0.0, -0.0},
                        rpy = {0., -0.8, -0.0},
                        T = 0.3,
                    },
                    WP_4 = {
                        pos = {0.45, -0.2, 0.20},
                        lin_vel = {0.00, 0.0, -0.2},
                        rpy = {0., -0.3, 0.},
                        T = 1.,
                    },
                    WP_5 = {
                        pos = {0.1, -0.2, -0.1},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                },
            },
        },
        action_level_arm = {
            L = {
                enable = true,
                init_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.1, 0.4, -0.1},
                        lin_vel = {0.0, 0.2, 0.2},
                        rpy = {0., 0., 1.3},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.1, 0.55, 0.1},
                        lin_vel = {0.0, 0.0, 0.0},
                        rpy = {0., 0.1, 1.5},
                        T = 1.,
                    },
                    
                },
                repeat_seq = {
                    seq_len = 1,
                    WP_1 = {
                        pos = {0.1, 0.55, 0.1},
                        lin_vel = {0.0, 0.0, 0.0},
                        rpy = {0., 0.1, 1.5},
                        T = 0.2,
                    },
                },
                exit_seq = {
                    seq_len = 2,
                    WP_1 = {
                        pos = {0.1, 0.4, -0.1},
                        lin_vel = {0.0, -0.2, -0.2},
                        rpy = {0., 0., 1.3},
                        T = 1.,
                    },
                    WP_2 = {
                        pos = {0.1, 0.2, -0.1},
                        lin_vel = {0., 0., 0.},
                        rpy = {0., 0., 0.},
                        T = 1.,
                    },
                },
            },
            R = {
                enable = false,
            },
        },
    },

    stance_mode_portal_collect = {
        portal_pair_key = "portal_for_stance_mode",
    },
    stance_mode_portal_publish = {
        portal_pair_key = "portal_for_stance_mode",
    },

    stance_planner = {
        record_data_ = true,
        left_foot_name_ = "left_foot_link",
        right_foot_name_ = "right_foot_link",

        use_waist_joint_planning = true,

        list_action = {"yewen_squat", "deep_squat", "dance"},
        action_yewen_squat = {
            init_segments = {"yewen_init","yewen_stretch_out"},
            repeat_segments = {"yewen_hold",},
            exit_segments = {"yewen_retract","yewen_exit"},
        },
        action_deep_squat = {
            init_segments = {"deep_squat_init",},
            repeat_segments = {"deep_squat_hold",},
            exit_segments = {"deep_squat_exit",},
        },

        action_dance = {
            init_segments = {"dance_init",},
            repeat_segments = {"dance_repeat",},
            exit_segments = {"dance_exit",},
        },

        

        action_seg_yewen_init = {
            ref_foot = "left",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    pos = {0., -0.02, 0.42},
                    lin_vel = {0., 0., 0.},
                    duration = 1.5,
                },

            },
            torso_seq = { -- disabled
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 2.,
                    pos = {0., 0., 0.5},
                    lin_vel = {0., 0., 0.},
                    rpy = {0., 0., -0.},
                },
            },
            foot_seq = { -- disabled
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 1.,
                    pos = {0., -0.2, 0.05},
                    lin_vel = {0., 0., 0.},
                    rpy = {0., 0., 0.1},
                },
            },
            delay_time = 1.,
        },

        action_seg_yewen_stretch_out = {
            ref_foot = "left",
            move_foot_contact_state = false,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    pos = {0.00, -0.02, 0.42},
                    lin_vel = {0., 0., 0.},
                    duration = 2.,
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 2.,
                    pos = {0., 0., 0.5},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0.0, -0.},
                },
            },
            foot_seq = {
                seq_len = 3,
                WP_1 = {
                    start_tic = 0.,
                    duration = 1.,
                    pos = {0., -0.2, 0.1},
                    lin_vel = {0., 0.0, 0.},
                    rpy = {0., 0., -0.0},
                },
                WP_2 = {
                    start_tic = 1.,
                    duration = 2.,
                    pos = {0.40, -0.1, 0.15},
                    lin_vel = {0.1, 0.0, 0.},
                    rpy = {0., -0.9, -0.0},
                },
                WP_3 = {
                    start_tic = 3.,
                    duration = 1.,
                    pos = {0.47, -0.05, 0.3},
                    lin_vel = {0., 0.0, 0.},
                    rpy = {0., -1.45, -0.0},
                },
            },
        },

        action_seg_yewen_hold = {
            ref_foot = "left",
            move_foot_contact_state = false,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    pos = {0.00, -0.02, 0.42},
                    lin_vel = {0., 0., 0.},
                    duration = 0.2,
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.2,
                    pos = {0., 0., 0.5},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0.0, -0.},
                },
            },
            foot_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.2,
                    pos = {0.47, -0.05, 0.3},
                    lin_vel = {0., 0.0, 0.},
                    rpy = {0., -1.45, -0.0},
                },
            },
        },

        action_seg_yewen_retract = {
            ref_foot = "left",
            move_foot_contact_state = false,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    pos = {0.00, -0.02, 0.42},
                    lin_vel = {0., 0., 0.},
                    duration = 2.,
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 2.,
                    pos = {0., 0., 0.5},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0.0, -0.},
                },
            },
            foot_seq = {
                seq_len = 3,
                WP_1 = {
                    start_tic = 0.,
                    duration = 1.,
                    pos = {0.40, -0.1, 0.15},
                    lin_vel = {-0.1, 0.0, 0.},
                    rpy = {0., -0.9, -0.0},
                },
                WP_2 = {
                    start_tic = 1.,
                    duration = 1.,
                    pos = {0., -0.2, 0.1},
                    lin_vel = {0., 0.0, 0.},
                    rpy = {0., 0., -0.0},
                },
                WP_3 = {
                    start_tic = 2.,
                    duration = 1.,
                    pos = {-0., -0.2, 0.0},
                    lin_vel = {0., 0.0, 0.},
                    rpy = {0., 0., -0.},
                },
                
            },
        },

        action_seg_yewen_exit = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    pos = {0.0, -0.0, 0.45},
                    lin_vel = {0., 0., 0.},
                    duration = 2.5,
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.5,
                    pos = {0., 0., 0.5},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., -0.0, -0.0},
                },

            },
            foot_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.5,
                    pos = {-0.18, -0.25, 0.248},
                    lin_vel = {0., 0.0, 0.},
                    rpy = {0., 0., -0.7},
                },
            },
        },

        action_seg_deep_squat_init = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 3.,
                    pos = {0.05, 0., 0.33},
                    lin_vel = {0., 0., 0.},
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 3.,
                    pos = {0.05, 0., 0.33},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0.8, 0.},
                },

            },
            foot_seq = {
                seq_len = 0,
            },
        },

        action_seg_deep_squat_hold = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.1,
                    pos = {0.05, 0., 0.33},
                    lin_vel = {0., 0., 0.},
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.1,
                    pos = {0.05, 0., 0.33},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0.8, 0.},
                },

            },
            foot_seq = {
                seq_len = 0,
            },
        },

        action_seg_deep_squat_exit = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 3.,
                    pos = {0.0, 0., 0.50},
                    lin_vel = {0., 0., 0.},
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 3.,
                    pos = {0.0, 0., 0.50},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0., 0.},
                },

            },
            foot_seq = {
                seq_len = 0,
            },
        },

        action_seg_dance_init = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.2,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.},
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.2,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0., 0.},
                },

            },
            foot_seq = {
                seq_len = 0,
            },
        },

        action_seg_dance_repeat = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 2,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.5,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.},
                },
                WP_2 = {
                    start_tic = 0.5,
                    duration = 0.5,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.},
                },
            },
            torso_seq = {
                seq_len = 2,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.5,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., -0.2, 0.2},
                },
                WP_2 = {
                    start_tic = 0.5,
                    duration = 0.5,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., -0.2, -0.2},
                },
            },
            foot_seq = {
                seq_len = 0,
            },
            waist_joint_seq = {
                seq_len = 2,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.5,
                    pos = -0.5,
                    vel = 0.,
                },
                WP_2 = {
                    start_tic = 0.5,
                    duration = 0.5,
                    pos = 0.5,
                    vel = 0.,
                },
            },
        },
        
        action_seg_dance_exit = {
            ref_foot = "center",
            move_foot_contact_state = true,
            com_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.7,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.},
                },
            },
            torso_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.7,
                    pos = {0.0, 0.0, 0.45},
                    lin_vel = {0., 0., 0.0},
                    rpy = {0., 0., 0.},
                },

            },
            foot_seq = {
                seq_len = 0,
            },
            waist_joint_seq = {
                seq_len = 1,
                WP_1 = {
                    start_tic = 0.,
                    duration = 0.7,
                    pos = 0.,
                    vel = 0.,
                },

            },
        },
    },

    commander1 = {
        record_data = true,
        n_joint = 23, 
        kp = kp_test,
        kd = kd_test,
        p = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0., 
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
        v = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0., 
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
        torq = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0., 
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
        wave_type = "slope",
        peak = ready_pos_test,
        period = 2.0,
        start_from_cur_state = true,
    },

    stance_wbc_common = {
        wbc_config_path = "common_wbc_path",
        -- ======================================= PVT模式相关 =======================================
    },
    stance_pvt = {
        urdf_path = "urdf_path",
        limit_pst = 0.98,
        damping_joint_kd = 2.0,
        ee_name = {
            "left_foot_link",
            "right_foot_link",
            "left_hand_link",
            "right_hand_link",
            "Waist",
        },
        ee_point = {
             0.015,  0.000, -0.030,
             0.015,  0.000, -0.030,
            -0.012,  0.213,  0.000,
            -0.012, -0.213,  0.000,
             0.000,  0.000,  0.000,
        },
        q_min = {
            -1.5708, -0.3491,
            -3.3161, -1.7453, -2.2689, -2.4435,
            -3.3161, -1.5708, -2.2689, -0.0000,
            -3.1416,
            -3.1416, -0.5236, -1.0472, -0.0000, -0.8727, -0.4363,
            -3.1416, -1.5708, -1.0472, -0.0000, -0.8727, -0.4363,
        },
        q_max = {
             1.5708,  1.2217,
             1.2217,  1.5708,  2.2689,  0.0000,
             1.2217,  1.9453,  2.2689,  2.4435,
             3.1416,
             3.1416,  1.5708,  1.0472,  2.3387,  1.3491,  1.4363,
             3.1416,  0.5236,  1.0472,  2.3387,  1.3491,  1.4363,
        },
        q_better_guess = {
             0.00,  0.00,
             0.60, -1.20,  0.00, -1.50,
             0.60,  1.20,  0.00,  1.50,
             0.00,
            -0.40, -0.03,  0.05,  0.70, -0.35,  0.03,
            -0.40,  0.03, -0.05,  0.70, -0.35, -0.03,
        },
        qdot_max = {
            13.0, 13.0,
            13.0, 13.0, 13.0, 13.0,
            13.0, 13.0, 13.0, 13.0,
            13.0,
            13.0, 13.0, 13.0, 13.0, 13.0, 13.0,
            13.0, 13.0, 13.0, 13.0, 13.0, 13.0,
        },
        constraint_type_names = {
            "ConstraintTypeFull",
            "ConstraintTypeFull",
            "ConstraintTypePosition",
            "ConstraintTypeOrientation",
            "ConstraintTypePosition",
            "ConstraintTypeOrientation",
            "ConstraintTypeOrientation",
        },
        constraint_body_names = {
            "left_foot_link",
            "right_foot_link",
            "left_hand_link",
            "left_hand_link",
            "right_hand_link",
            "right_hand_link",
            "Waist",
        },
        constratint_weight = {1.0, 1.0, 1.0, 0.05, 1.0, 0.05, 1.0},
        lambda = 0.001,
        max_steps = 100,
        step_tol = 1e-7,
        vel_level_ik_weight_R = 1.0e-4,
        kp_joint = {
            -- 5.0,  5.0,
            -- 10.0, 10.0, 10.0, 10.0,
            -- 10.0, 10.0, 10.0, 10.0,
            -- 100.0,
            -- 160., 160., 160., 160., 50.0, 50.0,
            -- 160., 160., 160., 160., 50.0, 50.0,

            2.0,  2.0,
            5.0, 5.0, 2.0, 1.0,
            5.0, 5.0, 2.0, 1.0,
            10.0,
            10., 10., 10., 10., 1.0, 1.0,
            10., 10., 10., 10., 1.0, 1.0,
        },
        kd_joint = {
            0.1, 0.1,
            0.1, 0.1, 0.1, 0.1,
            0.1, 0.1, 0.1, 0.1,
            0.5,
            1.0, 1.0, 1.0, 1.0, 0.1, 0.1,
            1.0, 1.0, 1.0, 1.0, 0.1, 0.1,
        },
        kp_joint_stand = {
            0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 
            0.0, 0.0, 0.0, 0.0, 
            0.0, 
            0.0, 0.0, 0.0, 0.0, 1.0, 1.0,
            0.0, 0.0, 0.0, 0.0, 1.0, 1.0,
        },
        kd_joint_stand = {
            0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 
            0.0, 0.0, 0.0, 0.0, 
            0.0, 
            0.0, 0.0, 0.0, 0.0, 0.1, 0.1,
            0.0, 0.0, 0.0, 0.0, 0.1, 0.1,
        },
        increase_pd_time = 0.03,
        decrease_pd_time = 0.02,
        leg_dof = 6,
        leg_joint_start_idx = {11, 17},
        use_feet_level_ctrl = true,
        use_feet_level_ctrl_support_foot_only = false,
        ankle_joint_types = {"pitch", "roll", "pitch", "roll"},
        ankle_joint_indexes = {15, 16, 21, 22},
        set_head = true,
        head_yaw_ref = 0.0,
        head_pitch_ref = 0.0,
        head_yaw_joint_idx = 0,
        head_pitch_joint_idx = 1,
    },
    stance_planner_wbc_convert = {
        feet_point_pos_local = {0.015,  0.000, -0.030,},
        left_hand_pos_local = {-0.012,  0.213,  0.000,},
        right_hand_pos_local = {-0.012, -0.213,  0.000,},
        left_foot_name = "left_foot_link",
        right_foot_name = "right_foot_link",
        left_hand_name = "left_hand_link",
        right_hand_name = "right_hand_link",
        joint_cnt = 23,

        use_waist_joint_planning = true,
        waist_joint_idx = 10,
        waist_name = "Waist",
        waist_pos_local = {0. ,0., 0.},
        urdf_path = "urdf_path",
    },

    common_legged_ekf = {
        config_path = "common_wbc_path",
    },

    legged_estimate_convert = {
        config_path = "common_wbc_path",
        feet_point_pos_local = {0.015,  0.000, -0.030,},
    },

    planner_pvt_switch = {
        disabled_behavior = "disable",
    },

    intercept_motor_cmd = {
        intercepted_motor_indexes_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        joint_cnt = 23,
    },

    rl_locomotion = {
        model_file = "lib/rma_locomotion_model",
        is_serial = true,
    },
    rl_speed = {
        model_file = "lib/rma_run_model",
        is_serial = true,
    },

    rl_footprint = {
        --model_file = "lib/footprint_tracker",
        model_file = "lib/ball_dribble",
        is_serial = true,
    },

    rl_parallel_mech_switch = {
        disabled_behavior = "disable",
    },

    rl_dummy_pvt = {
        n_joint = 23,
        kp = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0.,
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
        kd = {
            5., 5.,
            5., 5., 5., 5.,
            5., 5., 5., 5.,
            5.,
            50., 50., 50., 50., 5., 5.,
            50., 50., 50., 50., 5., 5.,
        },
        p = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0.,
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
        v = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0.,
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
        torq = {
            0., 0.,
            0., 0., 0., 0.,
            0., 0., 0., 0.,
            0.,
            0., 0., 0., 0., 0., 0.,
            0., 0., 0., 0., 0., 0.,
        },
    },

    rl_speed_phase_portal_collect = {
        portal_pair_key = "portal_for_rl_speed_phase",
    },
    rl_speed_phase_portal_publish = {
        portal_pair_key = "portal_for_rl_speed_phase",
    },

    rl_footprint_phase_portal_collect = {
        portal_pair_key = "portal_for_rl_footprint_phase",
    },
    rl_footprint_phase_portal_publish = {
        portal_pair_key = "portal_for_rl_footprint_phase",
    },

    custom_traj = require('custom_traj'),

    scheduler = {
        record_interval = 10
    },
    drawer = {
        drawer_backend = "NoBackend",
    },
    dds = {
        target_ip =  "192.168.10.101, 127.0.0.1",
    },
    
    record_manager = {
        record_backends = {
            -- "LCM",
            -- "LCM2DevSuite",
        },
        publish_channel_name = "CHANNEL_RECORD_TO_DEV_SUITE_WHITE",
        real_robot = true,
        server_url = "192.168.1.199:8080",
        robot_name = "test_joints",
        sampling_rate = 0.5,
        chart_keys = {
            -- global
            "global/t_call_ms",
            "global/t_exec_ms",
            "global/t_exec_ms_module_motion_state_publisher",
            "global/t_exec_ms_module_commander1",
            "global/t_exec_ms_module_parallel_mech_input",
            "global/t_exec_ms_module_parallel_mech_output",
            "global/t_exec_ms_module_joint_map_output",
            "global/t_exec_ms_module_joint_map_input",
            "global/t_exec_ms_module_simulator_out",
            "global/t_exec_ms_module_simulator_in",
            "global/t_exec_ms_module_common_legged_ekf",
            "global/t_exec_ms_module_legged_estimate_convert",
            "global/t_exec_ms_module_dcm_mode_portal_collect",
            "global/t_exec_ms_module_dcm_mode_portal_publish",
            "global/t_exec_ms_module_planner_pvt_switch",
            "global/t_exec_ms_module_command_manager",
            "global/t_exec_ms_module_dcm_planner",
            "global/t_exec_ms_module_dcm_wbc_common",
            "global/t_exec_ms_module_dcm_pvt",
            "global/t_exec_ms_module_dcm_planner_wbc_convert",
            "global/t_exec_ms_module_hand_planner",
            "global/t_exec_ms_module_hand_wbc_common",
            "global/t_exec_ms_module_hand_pvt",
            "global/t_exec_ms_module_hand_planner_wbc_convert",
            "global/t_exec_ms_module_kick_planner",
            "global/t_exec_ms_module_kick_wbc_common",
            "global/t_exec_ms_module_kick_pvt",
            "global/t_exec_ms_module_kick_planner_wbc_convert",
            "global/t_exec_ms_record",
            "global/t_exec_ms_draw",

            -- simulator_in
            "simulator_in/joint_cmd_pos",
            "simulator_in/joint_cmd_vel",
            "simulator_in/joint_cmd_tor",
            "simulator_in/joint_cmd_kp",
            "simulator_in/joint_cmd_kd",
            "simulator_in/torque_act",      -- only on webots
            "simulator_in/t",
            
            -- simulator_out
            "simulator_out/joint_fb_pos",
            "simulator_out/joint_fb_vel",
            "simulator_out/joint_fb_tor",
            "simulator_out/imu_eul",
            "simulator_out/imu_acc",
            "simulator_out/imu_rotvel",
            "simulator_out/torso_cmd_vx",
            "simulator_out/torso_cmd_vy",
            "simulator_out/torso_cmd_rotz",
            "simulator_out/motor_temp",
            "simulator_out/motor_err_code",
            "simulator_out/t",
            -- "simulator_out/imu_eul_before",
            -- "simulator_out/imu_acc_before",
            -- "simulator_out/imu_rotvel_before",
            -- "simulator_out/global_acc",
            -- "simulator_out/limit_flag",

            -- common_legged_ekf
            "common_legged_ekf/t",
            "common_legged_ekf/joint_pos",
            "common_legged_ekf/joint_vel",
            -- "common_legged_ekf/com_pos",
            -- "common_legged_ekf/com_vel",
            "common_legged_ekf/left_foot_link_contact_state",
            "common_legged_ekf/right_foot_link_contact_state",
            "common_legged_ekf/left_foot_link_force",
            "common_legged_ekf/right_foot_link_force",
            "common_legged_ekf/left_foot_link_force_prob",
            "common_legged_ekf/right_foot_link_force_prob",
        
            -- dcm_wbc_common
            "dcm_wbc_common/wbc_duration",
            "dcm_wbc_common/qp_solve_cost",
            "dcm_wbc_common/joint_power",
            "dcm_wbc_common/opt_torque_",
            "dcm_wbc_common/opt_force_",
            "dcm_wbc_common/opt_qddot_",
            "dcm_wbc_common/t",
            "dcm_wbc_common/cost",
            "dcm_wbc_common/floating_base_state_ref_pos",
            "dcm_wbc_common/floating_base_state_fb_pos",
            "dcm_wbc_common/floating_base_state_ref_rpy",
            "dcm_wbc_common/floating_base_state_fb_rpy",
            "dcm_wbc_common/floating_base_state_ref_lin_vel",
            "dcm_wbc_common/floating_base_state_fb_lin_vel",
            "dcm_wbc_common/floating_base_state_ref_ang_vel",
            "dcm_wbc_common/floating_base_state_fb_ang_vel",
            "dcm_wbc_common/com_traj_ref_pos",
            "dcm_wbc_common/com_traj_fb_pos",
            "dcm_wbc_common/com_traj_ref_vel",
            "dcm_wbc_common/com_traj_fb_vel",
            "dcm_wbc_common/left_foot_link_ref_pos",
            "dcm_wbc_common/left_foot_link_fb_pos",
            "dcm_wbc_common/left_foot_link_ref_rpy",
            "dcm_wbc_common/left_foot_link_fb_rpy",
            -- "dcm_wbc_common/left_foot_link_ref_lin_vel",
            -- "dcm_wbc_common/left_foot_link_fb_lin_vel",
            -- "dcm_wbc_common/left_foot_link_ref_ang_vel",
            -- "dcm_wbc_common/left_foot_link_fb_ang_vel",
            "dcm_wbc_common/right_foot_link_ref_pos",
            "dcm_wbc_common/right_foot_link_fb_pos",
            "dcm_wbc_common/right_foot_link_ref_rpy",
            "dcm_wbc_common/right_foot_link_fb_rpy",
            -- "dcm_wbc_common/right_foot_link_ref_lin_vel",
            -- "dcm_wbc_common/right_foot_link_fb_lin_vel",
            -- "dcm_wbc_common/right_foot_link_ref_ang_vel",
            -- "dcm_wbc_common/right_foot_link_fb_ang_vel",

            -- paraparallel_mech_output
            -- "paraparallel_mech_output/t",
            -- "paraparallel_mech_output/joint_parallel_pos_fb",
            -- "paraparallel_mech_output/joint_parallel_vel_fb",
            -- "paraparallel_mech_output/joint_serial_pos_fb",
            -- "paraparallel_mech_output/joint_serial_vel_fb",

            -- parallel_mech_input
            "parallel_mech_input/t",
            "parallel_mech_input/motor_pvt_parallel_p_des",

            -- dcm_planner
            "dcm_planner/t",
            "dcm_planner/lfoot_ref_td",
            -- "dcm_planner/lfoot_ref_pos_xyz",
            -- "dcm_planner/lfoot_ref_vel_xyz",
            -- "dcm_planner/lfoot_ref_rpy",
            -- "dcm_planner/lfoot_ref_ang_vel",
            "dcm_planner/rfoot_ref_td",
            -- "dcm_planner/rfoot_ref_pos_xyz",
            -- "dcm_planner/rfoot_ref_vel_xyz",
            -- "dcm_planner/rfoot_ref_rpy",
            -- "dcm_planner/rfoot_ref_ang_vel",
            -- "dcm_planner/com_ref_pos_xyz",
            -- "dcm_planner/com_ref_vel_xyz",
            -- "dcm_planner/torso_ref_rpy",
            -- "dcm_planner/torso_ref_ang_vel",
            "dcm_planner/gait_idx",
            "dcm_planner/stop_phase",
            "dcm_planner/speed_cmd",
            "dcm_planner/lfoot_td_probability_",
            "dcm_planner/rfoot_td_probability_",

            -- kick_planner
            "kick_planner/t",
            "kick_planner/lfoot_ref_td",
            "kick_planner/lfoot_ref_pos_xyz",
            "kick_planner/lfoot_ref_vel_xyz",
            "kick_planner/lfoot_ref_acc_xyz",
            "kick_planner/lfoot_ref_rpy",
            "kick_planner/lfoot_ref_ang_vel",
            "kick_planner/lfoot_ref_ang_acc",
            "kick_planner/rfoot_ref_td",
            "kick_planner/rfoot_ref_pos_xyz",
            "kick_planner/rfoot_ref_vel_xyz",
            "kick_planner/rfoot_ref_rpy",
            "kick_planner/rfoot_ref_ang_vel",
            "kick_planner/com_ref_pos_xyz",
            "kick_planner/com_ref_vel_xyz",
            "kick_planner/torso_ref_rpy",
            "kick_planner/torso_ref_ang_vel",
            "kick_planner/cur_stage",
           
            -- dcm_pvt
            "dcm_pvt/t",
            "dcm_pvt/joint_pos_fb_",
            "dcm_pvt/joint_vel_fb_",
            "dcm_pvt/is_IK_success_",
            "dcm_pvt/q_res_",
            "dcm_pvt/qdot_res_",
            "dcm_pvt/kp_output",
            "dcm_pvt/kd_output",
            "dcm_pvt/torque_output",
            "dcm_pvt/p_des_output",
            "dcm_pvt/v_des_output",
            "dcm_pvt/delta_q_norm",
            "dcm_pvt/error_norm",
            "dcm_pvt/num_steps",
            
            -- kick_pvt
            "kick_pvt/left_foot_pos_ref_",
            "kick_pvt/left_foot_pos_fb_",

            -- rl_locomotion
            "rl_locomotion/obs",
            "rl_locomotion/privileged",
            "rl_locomotion/action",
            "rl_locomotion/q_des",
            "rl_locomotion/t",
        },
    },
}

function options.init(arg1)
    is_real_bot = arg1

    if  is_real_bot then
        urdf_path = "/opt/booster/Gait/configs/V23_big_foot_serial.urdf"
        common_wbc_path = "/opt/booster/Gait/configs/config_v23.toml"
        stance_wbc_path = "/opt/booster/Gait/configs/config_v23.toml"

        options.stance_pvt.urdf_path = urdf_path
        options.common_legged_ekf.config_path = common_wbc_path
        options.legged_estimate_convert.config_path = common_wbc_path
        options.stance_wbc_common.wbc_config_path = stance_wbc_path
    else
        urdf_path_sim = "./configs/t1.urdf"
        common_wbc_path_sim = "./configs/config_v23_isaac.toml"
        stance_wbc_path_sim = "./configs/config_v23_isaac.toml"

        options.stance_pvt.urdf_path = urdf_path_sim
        options.stance_planner_wbc_convert.urdf_path = urdf_path_sim

        options.common_legged_ekf.config_path = common_wbc_path_sim
        options.legged_estimate_convert.config_path = common_wbc_path_sim
        options.stance_wbc_common.wbc_config_path = stance_wbc_path_sim
        -- options.drawer.drawer_backend = "WebotsDrawerBackend"

        -- options.dcm_planner.demo_ = "demo2"
        -- options.dcm_planner.demo_ = ""

        ----------------------------- demo2 start --------------------------------------
        -- options.dcm_planner.demo_step_ = {1.0, 10.0, 2.0, 12.0, 2.0, 7.0, 1.0,
        --                                     8.0, 1.0, 4.5, 2.0, 3.5, 2.0, 1.0, 
        --                                     2.0, 1.0, 2.0, 1.0, 2.0, 1.0, 1.0, 
        --                                     1.0, 14.0, 2.0, 9.5, 2.0, 14.0, 2.0, 
        --                                     10.0, 1.0, 12.0}

        -- -- options.dcm_planner.demo_step_ = {1.0, 10.0, 2.0, 12.0, 2.0, 7.0, 1.0,
        -- --                                     8.0, 1.0, 4.5, 2.0, 3.5, 2.0, 1.0, 
        -- --                                     2.0, 1.0, 2.0, 1.0, 2.0, 1.0, 1.0, 
        -- --                                     1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
        -- --                                     0.0, 0.0, 0.0}
        -- options.dcm_planner.demo_vel_x_ = {0.0, 0.2, 0.0, -0.1, 0.0, 0.0, 0.0,
        --                                     0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3,
        --                                     0.0, -0.2, 0.0, -0.2, 0.0, 0.3, 0.0,
        --                                     0.0, 0.16, 0.0, 0.0, 0.0, 0.155, 0.0,
        --                                     0.0, 0.0, 0.0}
                           
        -- options.dcm_planner.demo_vel_y_ = {0.0, 0.0, 0.0, 0.0, 0.0, 0.07, 0.0,
        --                                     -0.07, 0.0, 0.0, 0.0, 0.0, 0.0, -0.07,
        --                                     0.0, -0.07, 0.0, 0.07, 0.0, 0.07, 0.0,
        --                                     0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        --                                     -0.07, 0.0, -0.07}
        -- options.dcm_planner.demo_vel_z_ = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        --                                     0.0, 0.0, 0.7, 0.0, -0.7, 0.0, 0.0,
        --                                     0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        --                                     0.0, 0.19, 0.0, 0.7, 0.0, -0.185, 0.0,
        --                                     0.3, 0.0, -0.3}
        ------------------------------ demo2 end -----------------------------------------


        -- 0.5 米每秒速度行走
        -- options.dcm_planner.demo_step_ = {1.0, 50.0}
        -- options.dcm_planner.demo_gait_index_ = {0, 1}
     
        -- options.dcm_planner.demo_vel_x_ = {0.0, 0.45}
                           
        -- options.dcm_planner.demo_vel_y_ = {0.0, 0.0}
        -- options.dcm_planner.demo_vel_z_ = {0.0, 0.0}

        -- options.kick_pvt.kp_joint = {
        --     9.0, 9.0,
        --     0.0, 0.0, 0.0, 0.0,
        --     0.0, 0.0, 0.0, 0.0,
        --     0.0,
        --     250.0, 200.0, 160.0, 250.0,  20.0,  20.0,
        --     250.0, 200.0, 160.0, 250.0,  20.0,  20.0,
        -- }
        -- options.kick_pvt.kd_joint = {
        --     0.1, 0.1,
        --     0.0, 0.0, 0.0, 0.0,
        --     0.0, 0.0, 0.0, 0.0,
        --     0.0,
        --     20.0,  25.0,  5.0,  10.0,  0.3,  0.3,
        --     20.0,  25.0,  5.0,  10.0,  0.3,  0.3,
        -- }

        -- options.dcm_pvt.kp_joint = {
        --     5.0, 5.0,
        --     10.0, 10.0, 10.0, 10.0,
        --     10.0, 10.0, 10.0, 10.0,
        --     100.0,
        --     150.0, 120.0, 120.0, 150.0,  100.0,  100.0,
        --     150.0, 120.0, 120.0, 150.0,  100.0,  100.0,
        --     -- 0.0, 0.0, 0.0, 0.0,  5.0,  5.0,
        --     -- 0.0, 0.0, 0.0, 0.0,  5.0,  5.0,

        --     -- 0.0, 0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,
        --     -- 0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,  0.0,  0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,  0.0,  0.0,
        -- }
        -- options.dcm_pvt.kd_joint = { 
        --     0.1, 0.1,
        --     0.1, 0.1, 0.1, 0.1,
        --     0.1, 0.1, 0.1, 0.1,
        --     1.0,
        --     20.0,  12.0,  12.0,  20.0,  0.1,  0.1,
        --     20.0,  12.0,  12.0,  20.0,  0.1,  0.1,
        --     -- 0.0,  0.0,  0.0,  0.0,  0.1,  0.1,
        --     -- 0.0,  0.0,  0.0,  0.0,  0.1,  0.1,

        --     -- 0.0, 0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,
        --     -- 0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,  0.0,  0.0,
        --     -- 0.0, 0.0, 0.0, 0.0,  0.0,  0.0,
        -- }

        -- options.simulator_in = {
        --     -- desc: 表示是否记录数据
        --     -- scope: 
        --     -- unit: 
        --     record_data = true,

        --     -- desc: 与仿真软件交互时，控制算法的控制周期。应为正整数
        --     -- scope: 
        --     -- unit: ms
        --     step_len_ = controller_base_dt_ms,

        --     -- desc: 表示Webots中各个关节执行器（电机）的名字。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: 
        --     -- unit: 
        --     webots_motor_names_ = {              
        --         "AAHead_yaw", "Head_pitch",
        --         "Left_Shoulder_Pitch", "Left_Shoulder_Roll", "Left_Elbow_Pitch", "Left_Elbow_Yaw",
        --         "Right_Shoulder_Pitch", "Right_Shoulder_Roll", "Right_Elbow_Pitch", "Right_Elbow_Yaw",
        --         "Waist",

        --         "Left_Hip_Pitch", "Left_Hip_Roll", "Left_Hip_Yaw",
        --         "Left_Knee_Pitch", "Crank_Up_Left", "Crank_Down_Left",
                
        --         "Right_Hip_Pitch", "Right_Hip_Roll", "Right_Hip_Yaw",
        --         "Right_Knee_Pitch", "Crank_Up_Right", "Crank_Down_Right",
        --     },

        --     -- desc: 表示各个关节执行器（电机）的正方向。1表示正方向不变，-1表示正方向反向。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: 
        --     -- unit: 
        --     motor_direction_ = {
        --         1, 1, 
        --         1, 1, 1, 1, 
        --         1, 1, 1, 1, 
        --         1,
        --         1, 1, 1, 1, 1, 1, 
        --         1, 1, 1, 1, 1, 1,
        --     },        

        --     -- desc: 表示发送给各个关节执行器（电机）的力矩指令的最大值。应为正数。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: 
        --     -- unit: Nm
        --     motor_max_torque_value_ = {
        --         7.0, 7.0,
        --         36.0, 36.0, 36.0, 36.0,
        --         36.0, 36.0, 36.0, 36.0,
        --         60.0,
        --         90.0, 60.0, 60.0, 130.0, 36.0, 50.0,
        --         90.0, 60.0, 60.0, 130.0, 36.0, 50.0,
        --     },

        --     -- desc: 表示各个关节执行器（电机）的位置指令的最大值。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: [0, 3.14159]
        --     -- unit: Nm
        --     motor_max_position_value_ = {
        --         3.14159, 3.14159, 
        --         3.14159, 3.14159, 3.14159, 3.14159, 
        --         3.14159, 3.14159, 3.14159, 3.14159, 
        --         3.14159,
        --         3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 
        --         3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 
        --     },

        --     -- desc: 表示是否添加关节执行器（电机）力矩变化率限制
        --     -- scope: 
        --     -- unit: 
        --     add_motor_torque_dot_limit_ = true,

        --     -- desc: 表示关节执行器（电机）力矩变化率上限。应为正数。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: 
        --     -- unit: Nm*ms^(-1)
        --     motor_max_torque_dot_value_ = {
        --         20.0, 20.0,
        --         20.0, 20.0, 20.0, 20.0,
        --         20.0, 20.0, 20.0, 20.0,
        --         20.0,
        --         20.0, 20.0, 20.0, 20.0, 20.0, 20.0,
        --         20.0, 20.0, 20.0, 20.0, 20.0, 20.0,
        --     },
            
        --     -- desc：表示是否要在机器人上自动施加推力
        --     -- scope: 
        --     -- unit:
        --     enable_push_test_ = false,

        --     -- desc：表示箭头模型的.wbo文件的路径。可以是绝对路径，也可以是相对于supervisor controller的路径
        --     -- scope: 
        --     -- unit:
        --     arrow_file_path_ = "../robotics_control/src/tests/biped_test/worlds/Arrow.wbo",

        --     -- desc：表示施加的推力是否是随机的。若是，则推力大小范围不超过max_push_force_；若不是，则推力设为max_push_force_
        --     -- scope: 
        --     -- unit:
        --     apply_random_push_force_ = false,
            
        --     -- desc：表示施加推力大小的随机种子
        --     -- scope: 
        --     -- unit:
        --     push_force_random_seed_ = 1.0,

        --     -- desc: 表示施加推力的各个分量的最大绝对值，应为正数
        --     -- scope: 
        --     -- unit: N
        --     max_push_force_ = { -56.0, 37.0, 0.0 }, -- adjust footprint

        --     -- desc: 表示施加推力的作用位置范围，应为正数
        --     -- scope: 
        --     -- unit: m
        --     range_force_pos_ = { 0.0, 0.0, 0.0 },

        --     -- desc: 表示施加推力的作用位置偏置
        --     -- scope: 
        --     -- unit: m
        --     force_pos_offset_ = { 0.0, 0.0, 0.1 },

        --     -- desc: 表示绘制推力可视化模型时，力的模型长度与力的大小的比值
        --     -- scope: 
        --     -- unit: m/N
        --     force_length_scale_ = 0.01,

        --     -- desc: 表示施加推力的持续时间，应为正数
        --     -- scope: 
        --     -- unit: s
        --     -- force_action_time_ = 0.1,
        --     force_action_time_ = 0.2, -- for default
        --     -- force_action_time_ = 4.0,

        --     -- desc: 表示施加推力的时间间隔，应为正数，且大于force_action_time_
        --     -- scope: 
        --     -- unit: s
        --     force_time_interval_ = 5.0,
        --     -- force_time_interval_ = 4.0,
        --     -- force_time_interval_ = 3.0,
        -- }
        -- options.simulator_out = {
        --     -- desc: 表示是否记录数据
        --     -- scope: 
        --     -- unit: 
        --     record_data = true,

        --     -- desc: 表示Webots中各个关节的位置传感器的名字。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: 
        --     -- unit: 
        --     webots_motor_pos_sensor_names_ = {    
        --         "AAHead_yaw_sensor", "Head_pitch_sensor",
        --         "Left_Shoulder_Pitch_sensor", "Left_Shoulder_Roll_sensor", "Left_Elbow_Pitch_sensor", "Left_Elbow_Yaw_sensor",
        --         "Right_Shoulder_Pitch_sensor", "Right_Shoulder_Roll_sensor", "Right_Elbow_Pitch_sensor", "Right_Elbow_Yaw_sensor",
        --         "Waist_sensor",

        --         "Left_Hip_Pitch_sensor", "Left_Hip_Roll_sensor", "Left_Hip_Yaw_sensor",
        --         "Left_Knee_Pitch_sensor", "Crank_Up_Left_sensor", "Crank_Down_Left_sensor",
                
        --         "Right_Hip_Pitch_sensor", "Right_Hip_Roll_sensor", "Right_Hip_Yaw_sensor",
        --         "Right_Knee_Pitch_sensor", "Crank_Up_Right_sensor", "Crank_Down_Right_sensor",
        --     },

        --     -- desc: 表示各个关节的位置传感器的正方向。1表示正方向不变，-1表示正方向反向。各个分量对应各个关节。关节的顺序依次为：胸腔旋转、左肩的前摆、侧摆和肘关节、右肩的前摆、侧摆和肘关节、左腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角、右腿的前摆、侧摆、大腿旋转、膝盖、脚踝仰俯角、脚踝横滚角。
        --     -- scope: 
        --     -- unit: 
        --     motor_direction_ = {
        --         1, 1,
        --         1, 1, 1, 1,
        --         1, 1, 1, 1,
        --         1,
        --         1, 1, 1, 1, 1, 1, 
        --         1, 1, 1, 1, 1, 1,
        --     },

        --     -- desc: 表示Webots中各个IMU传感器的名字。顺序依次为：躯干、左脚板、右脚板
        --     -- scope: 
        --     -- unit: 
        --     webots_imu_sensor_names_ = {
        --         "torso inertial unit", 
        --     },

        --     -- desc: 表示Webots中各个陀螺仪传感器的名字。顺序依次为：躯干、左脚板、右脚板
        --     -- scope: 
        --     -- unit: 
        --     webots_gyro_sensor_names_ = {
        --         "torso gyro", 
        --     },

        --     -- desc: 表示Webots中各个加速度传感器的名字。顺序依次为：躯干、左脚板、右脚板
        --     -- scope: 
        --     -- unit: 
        --     webots_acc_sensor_names_ = {
        --         "torso accelerometer",
        --     },

        --     -- desc: 表示Webots中各个GPS传感器的名字。顺序依次为：躯干、左脚板、右脚板
        --     -- scope: 
        --     -- unit: 
        --     webots_gps_sensor_names_ = {
        --         "torso gps", 
        --     },

        --     -- desc: 表示Webots中各个力传感器的名字。顺序依次为：左脚板、右脚板
        --     -- scope: 
        --     -- unit: 
        --     webots_force_sensor_names_ = {
        --         -- "left touch sensor", "right touch sensor",
        --     },

        --     -- desc: 表示Webots中各个力矩传感器的名字。置空表示没有传感器
        --     -- scope: 
        --     -- unit: 
        --     webots_torque_sensor_names_ = {
        --         -- "", "", "", "", "", "",
        --     },
        -- }
    end
end

return options
