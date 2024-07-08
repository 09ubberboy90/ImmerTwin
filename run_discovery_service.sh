#!/bin/bash

source fastdds_setup.sh
source ~/.ros_profile
fastdds discovery -i 0 &
ros2 daemon start

