# `pico_ice` ros2 package

This directory is a ROS2 package built from the
[this ROS2 rclpy example](https://docs.ros.org/en/eloquent/Tutorials/Writing-A-Simple-Py-Publisher-And-Subscriber.html).

Once in this `ros2` directory, you can run the following commands to build it:

```
rosdep install --from-path src
colcon build
```

And then run the following to start it:

```
. install/setup.sh
ros2 run pico_ice talker # for receiving messages
ros2 run pico_ice listener # for sending messages
```
