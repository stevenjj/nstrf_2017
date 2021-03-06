#include "ik_ihmc_bridge.h"

IK_IHMC_Bridge::IK_IHMC_Bridge() : rarm_joint_names{"rightShoulderPitch", "rightShoulderRoll", "rightShoulderYaw", 
													"rightElbowPitch", "rightForearmYaw", "rightWristRoll", "rightWristPitch"}, 
								   larm_joint_names{"leftShoulderPitch",  "leftShoulderRoll", "leftShoulderYaw",
													"leftElbowPitch", "leftForearmYaw", "leftWristRoll", "leftWristPitch" },
								   nasa_left_arm_joint_names{"leftForearmYaw", "leftWristRoll", "leftWristPitch", 
								   							 "leftThumbRoll", "leftThumbPitch1", "leftThumbPitch2", "leftIndexFingerPitch1",
								   							 "leftMiddleFingerPitch1", "leftPinkyPitch1"},
								   nasa_right_arm_joint_names{"rightForearmYaw", "rightWristRoll", "rightWristPitch", 
								   							 "rightThumbRoll", "rightThumbPitch1", "rightThumbPitch2", "rightIndexFingerPitch1",
								   							 "rightMiddleFingerPitch1", "rightPinkyPitch1"}

{


} // End Constructor

IK_IHMC_Bridge::~IK_IHMC_Bridge(){} // End Destructor


void IK_IHMC_Bridge::set_init_IK_state(RobotState &start_state){
	ik_init_robot_state.robot_pose = start_state.robot_pose;
	ik_init_robot_state.joint_state = start_state.joint_state;
	ik_init_robot_state.valid_fields = start_state.valid_fields;
}
void IK_IHMC_Bridge::set_final_IK_state(RobotState &end_state){
	ik_final_robot_state.robot_pose = end_state.robot_pose;		
	ik_final_robot_state.joint_state = end_state.joint_state;	
	ik_final_robot_state.valid_fields = end_state.valid_fields;
}

bool IK_IHMC_Bridge::calc_single_hand_IK(const geometry_msgs::Pose& des_hand_pose, const int& robot_side, 
										 const RobotState& robot_state_input, RobotState& robot_state_output){
	// robot_side is not yet implemented

	if(robot_state_input.valid_fields){
		// Prepare Client Call
		val_ik::DrakeOneHandSingleIk single_ik_srv;

	    // Prepare Robot State msg
	    val_ik_msgs::RobotState robot_state_msg;
	    geometry_msgs::Pose robot_pose;
	    // Convert odometry message to geometry_msgs::Pose
	    robot_pose = robot_state_input.robot_pose.pose.pose; // This is because robot_pose is a nav_msgs::Odometry
	    robot_state_msg.robot_pose = robot_pose;
		robot_state_msg.body_joint_states = robot_state_input.joint_state;    

		single_ik_srv.request.robot_state = robot_state_msg;
		single_ik_srv.request.des_hand_pose = des_hand_pose;

		// Declare Robot Side
		single_ik_srv.request.robot_side = robot_side;

		if (single_ik_client.call(single_ik_srv)){
	        ROS_INFO("    Single IK Call Successful");
	        // Store to final IK position
	        robot_state_output.robot_pose.pose.pose = single_ik_srv.response.robot_state.robot_pose;
	        robot_state_output.joint_state = single_ik_srv.response.robot_state.body_joint_states;
	        robot_state_output.valid_fields = true;	
	        return true;
	    }
	    else{
	       ROS_WARN("    Failed to call Single IK service");
	       return false;
	    }
	}

	ROS_WARN("    Failed to call single IK service. Invalid initial robot state.");
	return false;
}

bool IK_IHMC_Bridge::FK_bodies( RobotState &robot_state,
		  					    std::vector<std::string> &body_queries, std::vector<geometry_msgs::Pose> &body_poses){
	val_ik::DrakeFKBodyPose fk_srv;
	val_ik_msgs::RobotState init_robot_state;

	init_robot_state.robot_pose = robot_state.robot_pose.pose.pose; // This is because robot_pose is a nav_msgs::Odometry
	init_robot_state.body_joint_states = robot_state.joint_state;

	// Fill in service request
	fk_srv.request.robot_state = init_robot_state;
	fk_srv.request.body_names = body_queries;

	if (fk_client.call(fk_srv)){
        ROS_INFO("    FK Call Successful");
		body_poses = fk_srv.response.body_world_poses;
		return true;
    }
    else{
       ROS_WARN("    Failed to call val_fk_service");
       return false;
    }
}


bool IK_IHMC_Bridge::prepareSingleIKWBC(RobotState &start_state, RobotState &end_state, double &traj_time,
						 ihmc_msgs::WholeBodyTrajectoryRosMessage &wbc_traj_msg,     
						 sensor_msgs::JointState &left_arm_msg,	
						 sensor_msgs::JointState &right_arm_msg,
						 int left_hand_open_close_status,
						 int right_hand_open_close_status){
	// Initialize ik positions
	set_init_IK_state(start_state);
	set_final_IK_state(end_state);	
	std::vector<std::string>::iterator it;

	int traj_unique_id = 1;

	if (!start_state.valid_fields){
		ROS_ERROR("Starting state passed is not valid");
		return false;
	}

	if (!end_state.valid_fields){
		ROS_ERROR("Ending state passed is not valid");
		return false;
	}	

	if (ik_init_robot_state.valid_fields && ik_final_robot_state.valid_fields){
		// Prepare NASA Right Arm Message
		right_arm_msg.name = nasa_right_arm_joint_names;

		// Set Fingers Angle
		float right_fingers_angle = OPEN_HAND_VALS;
		if (right_hand_open_close_status == CLOSE_HAND){
			right_fingers_angle = CLOSE_HAND_VALS;
		}

		// Fill in Position for all right arm joints. Overwrite Forearms later
		for (size_t i = 0; i < nasa_right_arm_joint_names.size(); i++){
			right_arm_msg.position.push_back(right_fingers_angle);		
			right_arm_msg.velocity.push_back(0.0);		
			right_arm_msg.effort.push_back(0.0);					 
		}

		// Set Forearms to 0.0 unless filled in later by IK
		for (size_t i = 0; i < 3; i++){
			right_arm_msg.position[i] = 0.0;		 
		}

	    // Begin Right Arm Trajectory Message ----------------------------------------------------------------------------------------------
	    ihmc_msgs::ArmTrajectoryRosMessage rarm_traj_msg;
	    std::cout << "Preparing Right Arm Trajectory Message" << std::endl;
		// Right Arm: Add initial and final points
		for (size_t i = 0; i < rarm_joint_names.size(); i++){
			// Begin right arm joint;
	        ihmc_msgs::OneDoFJointTrajectoryRosMessage rarm_joint;

			// Find Joint Index of Right Arm Joints for starting waypoint
			it = std::find (ik_init_robot_state.joint_state.name.begin(), ik_init_robot_state.joint_state.name.end(), rarm_joint_names.at(i));
			int sw_joint_index = std::distance(ik_init_robot_state.joint_state.name.begin(), it);
			// Set the joint values
			double joint_start_value = ik_init_robot_state.joint_state.position[sw_joint_index];


			// Find Joint Index of Right Arm Joints for ending waypoint
			it = std::find (ik_final_robot_state.joint_state.name.begin(), ik_final_robot_state.joint_state.name.end(), rarm_joint_names.at(i));
			int ew_joint_index = std::distance(ik_final_robot_state.joint_state.name.begin(), it);		
			double joint_end_value   = ik_final_robot_state.joint_state.position[ew_joint_index];

/*			std::cout << "  ik_init_size: " << ik_init_robot_state.joint_state.name.size() << std::endl;
			std::cout << "  SW Found joint " << ik_init_robot_state.joint_state.name[sw_joint_index]  << " val: " << joint_start_value << std::endl;*/

//			std::cout << "  ik_final_size: " << ik_final_robot_state.joint_state.name.size() << std::endl;
			std::cout << "  EW Found joint " << ik_final_robot_state.joint_state.name[ew_joint_index]  << " val: " << joint_end_value << std::endl; 			


			// Fill in "rightForearmYaw", "rightWristRoll", "rightWristPitch"
			for (size_t j = 0; j < 3; j++){
				if (right_arm_msg.name[j].compare(ik_final_robot_state.joint_state.name[ew_joint_index]) == 0){
					std::cout << "Found" << right_arm_msg.name[j] << " setting it to" <<  joint_end_value << std::endl;
					right_arm_msg.position[j] = joint_end_value;
				}
			}


            ihmc_msgs::TrajectoryPoint1DRosMessage joint_start_val_msg;
            ihmc_msgs::TrajectoryPoint1DRosMessage joint_end_val_msg; 
		
            joint_start_val_msg.time = 0.0;    
            joint_start_val_msg.position = joint_start_value;  
            joint_start_val_msg.velocity = 0.0; 
            joint_start_val_msg.unique_id = traj_unique_id;

            joint_end_val_msg.time = traj_time;      
            joint_end_val_msg.position = joint_end_value;      
            joint_end_val_msg.velocity = 0.0;            
            joint_end_val_msg.unique_id = traj_unique_id;

			rarm_joint.trajectory_points.push_back(joint_start_val_msg);
			rarm_joint.trajectory_points.push_back(joint_end_val_msg);
			rarm_joint.unique_id = traj_unique_id;
			
			#ifdef ON_REAL_ROBOT
				rarm_joint.weight = std::numeric_limits<double>::quiet_NaN(); // Added line to comply with new ihmc messages
			#endif

			rarm_traj_msg.robot_side = rarm_traj_msg.RIGHT; // RIGHT SIDE		
			rarm_traj_msg.joint_trajectory_messages.push_back(rarm_joint);
			rarm_traj_msg.execution_mode = 0;
			rarm_traj_msg.previous_message_id = 0;
			rarm_traj_msg.unique_id = traj_unique_id;

		}


		// Prepare NASA Left Arm Message
		left_arm_msg.name = nasa_left_arm_joint_names;
		// Set Fingers Angle
		float left_fingers_angle = OPEN_HAND_VALS;
		if (right_hand_open_close_status == CLOSE_HAND){
			left_fingers_angle = CLOSE_HAND_VALS;
		}

		// Fill in Position for all right arm joints. Overwrite Forearms later
		for (size_t i = 0; i < nasa_right_arm_joint_names.size(); i++){
			left_arm_msg.position.push_back(left_fingers_angle);	
			left_arm_msg.velocity.push_back(0.0);		
			left_arm_msg.effort.push_back(0.0);					 				 
		}

		// Set Forearms to 0.0 unless filled in later by IK
		for (size_t i = 0; i < 3; i++){
			left_arm_msg.position[i] = 0.0;		 
		}


	    std::cout << "Preparing Left Arm Trajectory Message" << std::endl;
	    // Begin Left Arm Trajectory Message ----------------------------------------------------------------------------------------------
	    ihmc_msgs::ArmTrajectoryRosMessage larm_traj_msg;
		// Left Arm: Add initial and final points
		for (size_t i = 0; i < larm_joint_names.size(); i++){
			// Begin right arm joint;
	        ihmc_msgs::OneDoFJointTrajectoryRosMessage larm_joint;

			// Find Joint Index of Left Arm Joints for starting waypoint
			it = std::find (ik_init_robot_state.joint_state.name.begin(), ik_init_robot_state.joint_state.name.end(), larm_joint_names.at(i));
			int sw_joint_index = std::distance(ik_init_robot_state.joint_state.name.begin(), it);
			// Set the joint values
			double joint_start_value = ik_init_robot_state.joint_state.position[sw_joint_index];


			// Find Joint Index of Left Arm Joints for ending waypoint
			it = std::find (ik_final_robot_state.joint_state.name.begin(), ik_final_robot_state.joint_state.name.end(), larm_joint_names.at(i));
			int ew_joint_index = std::distance(ik_final_robot_state.joint_state.name.begin(), it);		
			double joint_end_value   = ik_final_robot_state.joint_state.position[ew_joint_index];

/*			std::cout << "  SW index: " << sw_joint_index << std::endl;
			std::cout << "  SW Found joint " << ik_init_robot_state.joint_state.name[sw_joint_index]  << " val: " << joint_start_value << std::endl;
*/
//			std::cout << "  EW index: " << ew_joint_index << std::endl;
			std::cout << "  EW Found joint " << ik_final_robot_state.joint_state.name[ew_joint_index]  << " val: " << joint_end_value << std::endl; 			


			// Fill in "leftForearmYaw", "leftWristRoll", "leftWristPitch"
			for (size_t j = 0; j < 3; j++){
				if (left_arm_msg.name[j].compare(ik_final_robot_state.joint_state.name[ew_joint_index]) == 0){
					std::cout << "Found" << left_arm_msg.name[j] << " setting it to" <<  joint_end_value << std::endl;
					left_arm_msg.position[j] = joint_end_value;
				}
			}


            ihmc_msgs::TrajectoryPoint1DRosMessage joint_start_val_msg;
            ihmc_msgs::TrajectoryPoint1DRosMessage joint_end_val_msg; 

		
            joint_start_val_msg.time = 0.0;    
            joint_start_val_msg.position = joint_start_value;  
            joint_start_val_msg.velocity = 0.0; 
            joint_start_val_msg.unique_id = traj_unique_id;

            joint_end_val_msg.time = traj_time;      
            joint_end_val_msg.position = joint_end_value;      
            joint_end_val_msg.velocity = 0.0;            
            joint_end_val_msg.unique_id = traj_unique_id;

			larm_joint.trajectory_points.push_back(joint_start_val_msg);
			larm_joint.trajectory_points.push_back(joint_end_val_msg);
			larm_joint.unique_id = traj_unique_id;

			#ifdef ON_REAL_ROBOT
				larm_joint.weight = std::numeric_limits<double>::quiet_NaN(); // Added line to comply with new ihmc messages
			#endif			

			larm_traj_msg.robot_side = larm_traj_msg.LEFT; // LEFT SIDE		
			larm_traj_msg.joint_trajectory_messages.push_back(larm_joint);
			larm_traj_msg.execution_mode = 0;
			larm_traj_msg.previous_message_id = 0;
			larm_traj_msg.unique_id = traj_unique_id;
		}		

		// Copy Arm trajectories to Whole Body Message
		wbc_traj_msg.right_arm_trajectory_message = rarm_traj_msg;		
		wbc_traj_msg.left_arm_trajectory_message = larm_traj_msg;

		std::cout << "Hello world!" << std::endl;

		// GET FK for both initial and final states.
		std::vector<std::string> body_queries;
		body_queries.push_back("torso"); body_queries.push_back("pelvis");	

		std::vector<geometry_msgs::Pose> initial_body_poses;
		if(FK_bodies(ik_init_robot_state, body_queries, initial_body_poses) == false ){
			return false;
		}

		std::vector<geometry_msgs::Pose> final_body_poses;
		if(FK_bodies(ik_final_robot_state, body_queries, final_body_poses) == false ){
			return false;
		}

    	geometry_msgs::Vector3 angular_velocity; 
    	angular_velocity.x = 0.0;
		angular_velocity.y = 0.0;
		angular_velocity.z = 0.0;

		// Prepare Chest Trajectory Message
	    ihmc_msgs::ChestTrajectoryRosMessage chest_trajectory_msg;
        	ihmc_msgs::SO3TrajectoryPointRosMessage     start_SO3_chest_traj;       	
        	start_SO3_chest_traj.time = 0.0; // Start Time is 0
        	start_SO3_chest_traj.orientation = initial_body_poses[0].orientation;
        	start_SO3_chest_traj.angular_velocity = angular_velocity;
        	start_SO3_chest_traj.unique_id = traj_unique_id;

        	ihmc_msgs::SO3TrajectoryPointRosMessage     end_SO3_chest_traj;  
        	end_SO3_chest_traj.time = traj_time; // End Time is specified
        	end_SO3_chest_traj.orientation = final_body_poses[0].orientation;
        	end_SO3_chest_traj.angular_velocity = angular_velocity;
        	end_SO3_chest_traj.unique_id = traj_unique_id;

        chest_trajectory_msg.execution_mode = 0;
        chest_trajectory_msg.previous_message_id = 0;
        chest_trajectory_msg.unique_id = traj_unique_id;
        chest_trajectory_msg.taskspace_trajectory_points.push_back(start_SO3_chest_traj);
        chest_trajectory_msg.taskspace_trajectory_points.push_back(end_SO3_chest_traj);

		#ifdef ON_REAL_ROBOT
        	chest_trajectory_msg.frame_information.trajectory_reference_frame_id = 83766130;
        	chest_trajectory_msg.frame_information.data_reference_frame_id = 83766130;        	
	        chest_trajectory_msg.use_custom_control_frame = false;
		#endif


    	geometry_msgs::Vector3 linear_velocity; 
    	angular_velocity.x = 0.0;
		angular_velocity.y = 0.0;
		angular_velocity.z = 0.0;
        // Prepare Pelvis SE(3) Trajectory Message
        ihmc_msgs::PelvisTrajectoryRosMessage	pelvis_trajectory_message;
        	ihmc_msgs::SE3TrajectoryPointRosMessage 	start_SE3_pelvis_traj;
        	start_SE3_pelvis_traj.time = 0.0;
        	start_SE3_pelvis_traj.position.x = initial_body_poses[1].position.x;
        	start_SE3_pelvis_traj.position.y = initial_body_poses[1].position.y;
        	start_SE3_pelvis_traj.position.z = initial_body_poses[1].position.z;        	        	
        	start_SE3_pelvis_traj.orientation = initial_body_poses[1].orientation;
        	start_SE3_pelvis_traj.linear_velocity = linear_velocity;
        	start_SE3_pelvis_traj.angular_velocity = angular_velocity;  
        	start_SE3_pelvis_traj.unique_id = traj_unique_id;      	

        	ihmc_msgs::SE3TrajectoryPointRosMessage 	end_SE3_pelvis_traj;
        	end_SE3_pelvis_traj.time = traj_time;
        	end_SE3_pelvis_traj.position.x = final_body_poses[1].position.x;
        	end_SE3_pelvis_traj.position.y = final_body_poses[1].position.y;
        	end_SE3_pelvis_traj.position.z = final_body_poses[1].position.z;        	        	
        	end_SE3_pelvis_traj.orientation = final_body_poses[1].orientation;
        	end_SE3_pelvis_traj.linear_velocity = linear_velocity;
        	end_SE3_pelvis_traj.angular_velocity = angular_velocity;  
        	end_SE3_pelvis_traj.unique_id = traj_unique_id;

        pelvis_trajectory_message.execution_mode = 0;
        pelvis_trajectory_message.previous_message_id = 0;
        pelvis_trajectory_message.unique_id = traj_unique_id;
        pelvis_trajectory_message.taskspace_trajectory_points.push_back(start_SE3_pelvis_traj);
        pelvis_trajectory_message.taskspace_trajectory_points.push_back(end_SE3_pelvis_traj);        

		#ifdef ON_REAL_ROBOT
        	pelvis_trajectory_message.frame_information.trajectory_reference_frame_id = 83766130;
        	pelvis_trajectory_message.frame_information.data_reference_frame_id = 83766130;        	
	        pelvis_trajectory_message.use_custom_control_frame = false;
		#endif


        // Copy Chest and Pelvis Trajectories
        wbc_traj_msg.chest_trajectory_message = chest_trajectory_msg;
        wbc_traj_msg.pelvis_trajectory_message = pelvis_trajectory_message;        

		wbc_traj_msg.unique_id = traj_unique_id;
		wbc_traj_msg.right_foot_trajectory_message.robot_side = 1;
		wbc_traj_msg.right_hand_trajectory_message.robot_side = 1;

	}


}