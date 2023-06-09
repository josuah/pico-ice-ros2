# Copyright 2016 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy
from sensor_msgs.msg import Imu

from pico_ice import wishbone_serial


tty = '/dev/ttyACM1'


class PicoIceSubscriber(Node):

    def __init__(self):
        super().__init__('pico_ice_subscriber')
        qos = QoSProfile(
            depth=10,
            durability=DurabilityPolicy.SYSTEM_DEFAULT,
            reliability=ReliabilityPolicy.SYSTEM_DEFAULT
        )
        self.subscription = self.create_subscription(
            Imu, '/imu', self.listener_callback, qos)
        self.subscription  # prevent unused variable warning

    def listener_callback(self, imu):
        self.get_logger().info(f'({imu.orientation.x},{imu.orientation.z},{imu.orientation.z})')
        wishbone_serial.write(tty, 0x1000, 1000 * int(imu.orientation.x))
        wishbone_serial.write(tty, 0x1001, 1000 * int(imu.orientation.y))
        wishbone_serial.write(tty, 0x1002, 1000 * int(imu.orientation.z))

def main(args=None):
    rclpy.init(args=args)
    pico_ice_subscriber = PicoIceSubscriber()
    rclpy.spin(pico_ice_subscriber)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
