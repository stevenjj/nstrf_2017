#include "logic_main.h"
// Declare Global Objects
bool global_state_update_received = false;
bool global_first_state_update_received = false;
boost::mutex state_mutex;
sensor_msgs::JointState      global_joint_state_msg;
nav_msgs::Odometry           global_odom_msg;


int main(int argc, char** argv){
    // Initialize ROS
    ros::init(argc, argv, "val_logic_manager");
    LogicManager logic_manager;

    // Declare Service Call clients
    // IK Client
    logic_manager.ik_client = logic_manager.nh.serviceClient<val_ik::DrakeIKVal>("val_ik/val_ik_service");

    // Declare Subscribers
    logic_manager.interactive_marker_sub = logic_manager.nh.subscribe<visualization_msgs::InteractiveMarkerInit>("/basic_controls/update_full", 1, 
                                                                                                                  boost::bind(&LogicManager::interactive_callback, &logic_manager, _1));
    logic_manager.operator_command_sub = logic_manager.nh.subscribe<std_msgs::String>("val_logic_manager/operator_command", 1,
                                                                                       boost::bind(&LogicManager::operator_command_callback, &logic_manager, _1));
    // Synchronize Robot Joint State and Robot Pose
    message_filters::Subscriber<sensor_msgs::JointState> joint_state_sub(logic_manager.nh, "/ihmc_ros/valkyrie/output/joint_states", 1);
    message_filters::Subscriber<nav_msgs::Odometry> robot_pose_sub(logic_manager.nh, "/ihmc_ros/valkyrie/output/robot_pose", 1);

    // ApproximateTime takes a queue size as its constructor argument, hence JointOdomSyncPolicy(10)
    message_filters::Synchronizer<JointOdomSyncPolicy> sync(JointOdomSyncPolicy(10), joint_state_sub, robot_pose_sub);
    sync.registerCallback(boost::bind(&stateFiltersCallback, _1, _2));

    // Declare Publishers
    logic_manager.val_ik_finalpose_robot_joint_states_pub = logic_manager.nh.advertise<sensor_msgs::JointState>("/val_ik_finalpose_robot/joint_states", 0);
    logic_manager.val_ik_initpose_robot_joint_states_pub = logic_manager.nh.advertise<sensor_msgs::JointState>("/val_ik_initpose_robot/joint_states", 0);
    logic_manager.marker_pub = logic_manager.nh.advertise<visualization_msgs::Marker>("val_logic_manager/sample_marker", 0);

    ros::Rate r(20);

    // Spin Forever
    while (ros::ok()){
        logic_manager.loop();

        r.sleep();
        ros::spinOnce();        
    }

    return 0;
}