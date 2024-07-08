# ImmerSim-UE

## Documentation

### Setup and run

Make sure you have built Unreal Engine from source. SHould work with every version of Unreal Engine 5. Tested with Unreal 5.4

1.  Clone this repo : `git clone --recurse-submodules git@github.com:09ubberboy90/ImmerSim-UE.git`
2.  Retrieve the large files : `git-lfs pull && git submodule foreach git-lfs pull`
3.  Build and run
    ```
    cd ImmerTwin
    export UE5_DIR=<path to UE5>
    ./update_project_files.sh
    make ImmerTwinEditor
    source ./fastdds_setup.sh
    {unreal_engine_path} ImmerTwin.uproject
    ```
Since the project is set to use 
[ROS2 with Discovery Server](https://docs.ros.org/en/foxy/Tutorials/Advanced/Discovery-Server/Discovery-Server.html)
to communicate with ROS2 Node in UE, you needs to execute `./run_discovery_service.sh` before starting the editor in order to communicate with ROS externally. In every terminals you need to run `source ./fastdds_setup.sh`.

### Moving the robot

The object RobotTarget send continuously a Pose msg to `/bp/{robot}/{left or right}_hand/pose`. This message can then be used by some motion planning algorithm such as moveit or [Telesim](https://github.com/cvas-ug/telesim_pnp). The robot mimics the joint sents to Unreal `joint_states` by default. It does not need to be a real robot, as a robot joint publisher will work the same way. 

## Using a Real Robot

## Author
Florent Audonnet (https://github.com/09ubberboy90) - Researcher at University of Glasgow
