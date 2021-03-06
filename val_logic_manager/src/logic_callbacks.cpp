#include "logic_main.h"

void  LogicManager::interactive_callback(const visualization_msgs::InteractiveMarkerInitConstPtr& msg){
	ROS_INFO("Logic Manager Interactive Marker Callback received");	

	if (msg->markers.size() > 0){
		geometry_msgs::Pose 	   marker_pose;
		marker_pose = msg->markers[0].pose;

		// Publish a Transform for Visualization
		tf::Transform transform;
		tf::Quaternion q(msg->markers[0].pose.orientation.x, msg->markers[0].pose.orientation.y, msg->markers[0].pose.orientation.z, msg->markers[0].pose.orientation.w );
		transform.setOrigin( tf::Vector3(msg->markers[0].pose.position.x, msg->markers[0].pose.position.y, msg->markers[0].pose.position.z) );
		transform.setRotation(q);
		br.sendTransform(tf::StampedTransform(transform, ros::Time::now(), "world", "basic_controls/im_frame"));


		// Check that we have received at least a single initialized ik 
		if	(global_first_state_update_received and ik_init_robot_state.valid_fields){
			// Prepare initial IK robot state
		    ik_init_robot_state.joint_state = current_robot_state.joint_state;
		    ik_init_robot_state.robot_pose = current_robot_state.robot_pose;

		    // Calculate IK
			if (ik_manager.calc_single_hand_IK(marker_pose, hand_to_use, ik_init_robot_state, ik_final_robot_state)){
				ROS_INFO("calc_single_hand_success!");
				publish_ik_final_state_viz();
			}else{
				ROS_WARN("Failed to calculate desired hand pose IK");
			}

		}

	}

}

void  LogicManager::operator_command_callback(const std_msgs::StringConstPtr& msg){
	ROS_INFO("Logic Manager Operator Command Callback");	
	std::string send_single_ik_wbc;		 send_single_ik_wbc = "send_single_ik";
	std::string testFK;					 testFK = "testFK";
	std::string go_home;				 go_home = "go_home";
	std::string re_init_markers;		 re_init_markers = "re_init_markers";
	std::string run_grasploc;            run_grasploc = "run_grasploc";
	std::string get_nearest_grasp_ik;    get_nearest_grasp_ik = "get_nearest_grasp_ik";
	std::string try_next_grasp_ik;       try_next_grasp_ik = "try_next_grasp_ik";
	std::string use_right_hand;       	 use_right_hand = "use_right_hand";
	std::string use_left_hand;       	 use_left_hand = "use_left_hand";	
	std::string stop_all_trajectories;	 stop_all_trajectories = "stop_all_trajectories";


	if (send_single_ik_wbc.compare(msg->data) == 0){
		ROS_INFO("Attempting to send single IK WBC");
		sendSingleIKWBC();
	}else if(testFK.compare(msg->data) == 0){
		ROS_INFO("Attempting to call FK");
		std::vector<std::string> body_queries;
		std::vector<geometry_msgs::Pose> body_poses;

		body_queries.push_back("torso"); body_queries.push_back("rightPalm");				
		ik_manager.FK_bodies(ik_init_robot_state, body_queries, body_poses);       

	}else if(go_home.compare(msg->data) == 0){
		ROS_INFO("Sending Robot Home (to neutral position for walking)");
		sendWBCGoHome();
	}else if(re_init_markers.compare(msg->data) == 0){
		ROS_INFO("Interactive Markers are being Reset. IM server will handle it");
	}else if(run_grasploc.compare(msg->data) == 0){
		ROS_INFO("Calling Grasploc. Grasploc server will handle it");
	}else if(get_nearest_grasp_ik.compare(msg->data) == 0){
		ROS_INFO("Finding IK For nearest Grasp");
		try_grasp(0);
	}else if(try_next_grasp_ik.compare(msg->data) == 0){
		ROS_INFO("Find IK for next Grasp");

		if (hand_to_use == RIGHT_HAND){		
			if (right_hand_grasps.size() > 0){
			    righthand_grasp_index = (righthand_grasp_index + 1) % right_hand_grasps.size();
				try_grasp(righthand_grasp_index);
			}else{
				ROS_ERROR("There are no right hand stored grasps.");
			}
		}
		else if (hand_to_use == LEFT_HAND){

			if (left_hand_grasps.size() > 0){
			    lefthand_grasp_index = (lefthand_grasp_index + 1) % left_hand_grasps.size();
				try_grasp(lefthand_grasp_index);
			}else{
				ROS_ERROR("There are no left hand stored grasps.");
			}
		}


	}else if(use_right_hand.compare(msg->data) == 0){
		ROS_INFO("Will use right hand");
		hand_to_use = RIGHT_HAND;
	}else if(use_left_hand.compare(msg->data) == 0){
		ROS_INFO("Will use left hand");
		hand_to_use = LEFT_HAND;		
	}else if(stop_all_trajectories.compare(msg->data) == 0){
		ROS_INFO("Sending stopping all trajectories command");
		ihmc_msgs::StopAllTrajectoryRosMessage stop_traj_msg;
		ihmc_msgs::AbortWalkingRosMessage abort_walk_msg;

		stop_traj_msg.unique_id = 10;
		abort_walk_msg.unique_id = 11;

		ihmc_stop_all_traj_pub.publish(stop_traj_msg);
        ros::Duration(0.5).sleep();
        ihmc_abort_walking_pub.publish(abort_walk_msg);


	}		
	else{
		ROS_WARN("Unknown Operator Command");
	}


}

void LogicManager::grasploc_callback(const valkyrie::GraspHandPosesConstPtr& msg){
	ROS_INFO("Received Grasploc Callback. Storing New Grasps");

	right_hand_grasps.clear();
	std::cout << "Number of right hand grasps:" << msg->right_hand_pose.size() << std::endl;
	for(size_t i = 0; i < (msg->right_hand_pose.size()); i++){
		right_hand_grasps.push_back( msg->right_hand_pose[i]);	
	}
	ROS_INFO("Stored: %zu right hand grasps", right_hand_grasps.size());

	left_hand_grasps.clear();
	std::cout << "Number of left hand grasps:" << msg->left_hand_pose.size() << std::endl;
	for(size_t i = 0; i < (msg->left_hand_pose.size()); i++){
		left_hand_grasps.push_back( msg->left_hand_pose[i]);
	}	
	ROS_INFO("Stored: %zu left hand grasps", right_hand_grasps.size());	






} //grasploc

void stateFiltersCallback(const sensor_msgs::JointStateConstPtr& joint_state_msg, const nav_msgs::OdometryConstPtr& odom_msg){
	state_mutex.lock();
		global_joint_state_msg = (*joint_state_msg);
		global_odom_msg = (*odom_msg);
		global_first_state_update_received = true;
		global_state_update_received = true;		
	state_mutex.unlock();
}