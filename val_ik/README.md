# val_ik
ROS package for using drake's IK solver for the Valkyrie robot.

First ensure you have drake installed. Specifically, I am using this forked version https://github.com/stevenjj/drake/tree/my_dev_space
This repo has a tag called `working_examples` which is the last known commit with a working example of solving valkyrie's IK using Drake.


## Test Drake and Valkyrie IK
````
$ cd [drake workspace] (mine is located at ~/dev/drake_catkin_workspace/)
# Run the visualizer
$./install/bin/drake-visualizer 

# Test the IK
$./build/drake/drake/examples/Valkyrie/test/valkyrie_ik_test

# It should have passed If you have the visualizer open, you should see valkyrie in a standing position.
````

## Compile ROS node
change `set(DRAKE_CATKIN_WS /home/stevenjj/dev/drake_catkin_workspace)` in the CMakeLists.txt
````
cd [ros workspace]
catkin build
````

## Test ROS node
Run the visualizer as before, then:
````
# source your workspace
$rosrun val_ik val_ik
````

# Test the IK Trajectory:
````
roslaunch val_ik test_simple_ik_traj.launch
````
![alt text](https://raw.githubusercontent.com/stevenjj/nstrf_2017/master/val_ik/val_ik_trajectory_overlay_sample.gif)
