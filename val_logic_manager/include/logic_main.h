#ifndef LOGICMAIN_H
#define LOGICMAIN_H

#include <ros/ros.h>

// Standard Messages
#include "std_msgs/String.h"
#include "sensor_msgs/JointState.h"
#include "nav_msgs/Odometry.h"
#include "geometry_msgs/Pose.h"
#include "visualization_msgs/Marker.h"
#include "visualization_msgs/InteractiveMarkerInit.h"

// Message Filters to Synchronize Pose and Joint States
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

// TF Broadcaster
#include <tf/transform_broadcaster.h>

// Logic Manager Include File
#include "robot_state.h"

// Val IK Messages
#include "val_ik_msgs/BodyPositionConstraint.h"
#include "val_ik_msgs/BodyQuaternionConstraint.h"
#include "val_ik_msgs/JointPositionConstraint.h"
#include "val_ik_msgs/RobotJointStates.h"
#include "val_ik_msgs/RobotState.h"

//IHMC Control Messages
#include "ihmc_msgs/WholeBodyTrajectoryRosMessage.h"
#include "ihmc_msgs/GoHomeRosMessage.h"
#include "ihmc_msgs/StopAllTrajectoryRosMessage.h"
#include "ihmc_msgs/AbortWalkingRosMessage.h"


// Include ROS Service
#include "val_ik/DrakeIKVal.h"
#include "val_ik/DrakeOneHandSingleIk.h"

#include "ik_ihmc_bridge.h"

// Grasploc messages
#include "valkyrie/GraspHandPoses.h" //grasploc

// Helper
#include "quat_helper.h"



class LogicManager{
public:
    // Declare Node Handle
    ros::NodeHandle 					nh;
	ros::Publisher						val_ik_initpose_robot_joint_states_pub;	
	ros::Publisher						val_ik_finalpose_robot_joint_states_pub;	
	ros::Publisher						marker_pub;		

	// IHMC Controller Interface Publishers
	ros::Publisher 						ihmc_wholebody_pub;
	ros::Publisher 						ihmc_go_home_pub;	
	ros::Publisher 						ihmc_pelvis_traj_pub;
	ros::Publisher 						ihmc_stop_all_traj_pub;	
	ros::Publisher 						ihmc_abort_walking_pub;	

	// NASA Controller Interface Publishers
	ros::Publisher 						nasa_left_arm_pub;	
	ros::Publisher 						nasa_right_arm_pub;	


	ros::Subscriber 					interactive_marker_sub;
	ros::Subscriber 					operator_command_sub;	
	ros::Subscriber 					grasploc_sub; //grasploc


	IK_IHMC_Bridge 						ik_manager;

	RobotState 							current_robot_state;	
	RobotState 							ik_init_robot_state;	
	RobotState 							ik_final_robot_state;	


	visualization_msgs::Marker			sample_object_marker;
	tf::TransformBroadcaster 			br;

	int 								righthand_grasp_index;
	std::vector<geometry_msgs::Pose> 	right_hand_grasps; 

	int 								lefthand_grasp_index;
	std::vector<geometry_msgs::Pose> 	left_hand_grasps; 


	int 								hand_to_use;
	int 								left_hand_open_close_status;	
	int 								right_hand_open_close_status;		
	


	void init_sample_object_marker(); //initialize sample_marker
	void update_current_robot_state();
	void publish_ik_init_state_viz(); // the initial IK pose


//	void publish_ik_traj_state_viz(); // the IK trajectory
	void publish_ik_final_state_viz(); // the final IK pose



	void try_grasp(int index);

	void sendSingleIKWBC(); // Send Single IK solution to IHMC controller	
	void sendWBC(); // Send WBC to IHMC controller	
	void sendWBCGoHome(); // Send go Home message to IHMC controller

	// Node Callbacks
	void  interactive_callback(const visualization_msgs::InteractiveMarkerInitConstPtr& msg);
	void  operator_command_callback(const std_msgs::StringConstPtr& msg);



	void grasploc_callback(const valkyrie::GraspHandPosesConstPtr& msg); //grasploc

	// Loop
	void loop();

	// Constructor and Destructor
	LogicManager();
	~LogicManager();	
};

// Declare Existence of Global Variables (this is mainly to work with message_filters. Otherwise I should not be doing this)
extern boost::mutex 							state_mutex;
extern sensor_msgs::JointState					global_joint_state_msg;
extern nav_msgs::Odometry 						global_odom_msg;
extern bool 									global_state_update_received;
extern bool 									global_first_state_update_received;

// Declare Sync Policy
typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::JointState, nav_msgs::Odometry> JointOdomSyncPolicy;

// Declare Global Funcs
void  stateFiltersCallback(const sensor_msgs::JointStateConstPtr& joint_state_msg, const nav_msgs::OdometryConstPtr& odom_msg);


#endif