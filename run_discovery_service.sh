#!/bin/bash

source fastdds_setup.sh
fastdds discovery -i 0 &
ros2 daemon start

