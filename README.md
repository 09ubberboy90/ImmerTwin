# ImmerTwin

## Documentation

### Setup and run

Make sure you have built Unreal Engine from source. SHould work with every version of Unreal Engine 5. Tested with Unreal 5.4

1. Clone this repo : `git clone --recurse-submodules git@github.com:09ubberboy90/ImmerSim-UE.git`
2. Retrieve the large files : `git-lfs pull && git submodule foreach git-lfs pull`
3. Update the ip address and file location in `fastdd_config.xml` and `fastdds_setup.sh`
4. Update Unreal engine location in `update_project_files.sh`
5. Build and run

    ```bash
    cd ImmerTwin
    ./update_project_files.sh
    make ImmerTwinEditor
    source ./fastdds_setup.sh
    {unreal_engine_path} ImmerTwin.uproject
    ```

Since the project is set to use [ROS2 with Discovery Server](https://docs.ros.org/en/foxy/Tutorials/Advanced/Discovery-Server/Discovery-Server.html)
to communicate with ROS2 Node in UE, you needs to execute `./run_discovery_service.sh` before starting the editor in order to communicate with ROS externally. In every terminals you need to run `source ./fastdds_setup.sh`.

## Controlling the robot

The object RobotTarget send continuously a Pose msg to `/bp/{robot_ns}/{left or right}_hand/pose`. This topic can be updating by changing the PosePublisher and TriggerPub. The RobotTarget can also change color based on the force feedback of the robot. This can be configure in the ForceSub Element.

This message can then be used by some motion planning algorithm such as moveit or [Telesim](https://github.com/cvas-ug/telesim_pnp). The robot mimics the joint sent to Unreal `joint_states` by default. It does not need to be a real robot, as a robot joint publisher will work the same way. The joint listener can be updated by changing the Robot Interface related to the robot available in `/Blueprints`

## Adding a new robot

The robot 3D files must be imported and scaled into Unreal Engine. Then a new child blueprint of `RRBase Robot` must be created. The meshes must be added as `Static Mesh Component`. Joints are represented as `RRKinematic Joint Component` and must be integrated as child and parent of the 2 meshes it is linking.

Then all the meshes and joints needs to be initialized in the construction script. It is recommended to use one of the available robot as a base for this step either the UR3 or Baxter as the UR5e is deprecated and as not yet been updated.

Finally, a new Blueprint with `RRRobot ROS2Interface` as a parent needs to be created, which will be used to set the topics the robot must listen to, and set to be used by the robot ROS2Interface

### Adding a new Gripper

New gripper can be added following the same procedure as a robot and afterward setting it as a child Actor of the robot

In this case a new Robot Target must be created by creating a child blueprint to `GeneralTarget` and updating the mesh and offset

## Author

Florent Audonnet (<https://github.com/09ubberboy90>) - Researcher at University of Glasgow
