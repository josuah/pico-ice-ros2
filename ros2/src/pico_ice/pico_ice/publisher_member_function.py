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
from std_msgs.msg import String
import std_msgs.msg as message

from pico_ice import wishbone_serial


tty = '/dev/ttyACM1'


class PicoIcePublisher(Node):

    def __init__(self, addr):
        super().__init__('pico_ice_publisher')
        self.publisher_ = self.create_publisher(String, 'topic', 10)
        self.timer = self.create_timer(0.5, self.timer_callback)
        self.addr = addr

    def timer_callback(self):
        msg = String()
        value = wishbone_serial.read(tty, self.addr)
        msg.data = f'{value}'
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing: "%s"' % msg.data)


def main(args=None):
    rclpy.init(args=args)
    pico_ice_publisher = PicoIcePublisher(0x1001)
    rclpy.spin(pico_ice_publisher)
    rclpy.shutdown()
    help(message)


if __name__ == '__main__':
    main()
