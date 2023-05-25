# pico-ice integration in ROS2

pico-ice
| [Doc](http://pico-ice.tinyvision.ai/)
| [Hardware](https://github.com/tinyvision-ai-inc/pico-ice)
| [Software](https://github.com/tinyvision-ai-inc/pico-ice-sdk)
| [Schematic](https://raw.githubusercontent.com/tinyvision-ai-inc/pico-ice/main/Board/Rev3/pico-ice.pdf)
| [Assembly](https://htmlpreview.github.io/?https://github.com/tinyvision-ai-inc/pico-ice/blob/main/Board/Rev3/bom/ibom.html)
| [Discord](https://discord.gg/t2CzbAYeD2)

[![LectronZ](https://lectronz.com/static/badges/buy-it-on-lectronz-medium.png)](https://lectronz.com/stores/tinyvision-ai-store)
[![Tindie](https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-smalls.png)](https://www.tindie.com/stores/tinyvision_ai/?ref=offsite_badges&utm_source=sellers_vr2045&utm_medium=badges&utm_campaign=badge_small%22%3E)


## Initial Situation

In the context of ROS2, there is a central protocol that makes everything
communicate together. Anything using a different protocol is "bridged" to it.

Pushing this limit further Micro-ROS is bringing the protocol onto the
firmware itself, allowing ROS2-formatted messages sent directly over USB.

But this requires knowing the internals of ROS2 to be able to do anything
which limits the adopters to FPGA developers who are also well used to ROS2.

```
... <--ros2--> [agent] <--ros2--> [RP2040] <--???--> [iCE40]
                          ^^^^     ^^^^^^
                        difficult to debug
```

## Solution Proposed

To avoid any dependency on ROS2 for developping/debugging/building new
pico-ice FPGA HDL code, it is possible to use a trivial protocol such as
[spibone](https://github.com/xobs/spibone) that interfaces the FPGA bus
data registers directly over USB.

```
Read protocol:
    Write: 01 | AA | AA | AA | AA
    [Wishbone Operation]
    Read:  01 | VV | VV | VV | VV
Write protocol:
    Write: 00 | AA | AA | AA | AA | VV | VV | VV | VV
    [Wishbone Operation]
    Read:  00
```

A protocol looking like `spibone` above would be looking familiar to FPGA
developers, who would be the one working with it, providing a reference
of address for use by the ROS2 developers (possibly the same person).

```
... <--ros2--> [script.py] <--spibone--> [RP2040] <--spibone--> [iCE40]
                ^^^^^^^^^     ^^^^^^^     ^^^^^^     ^^^^^^^
                 easier to debug at every link of the chain
```


## ROS2 Integration

To allow ROS2 integration, a simple wrapper script in python would handle
the FPGA communication over USB on one side (`f = open('/dev/ttyACM0')`)
and the ROS2 communication on the other side
([`rclpy`](https://github.com/ros2/rclpy)).

On the left, FPGA bus addresses. On the right, ROS2 topic strings.

```python
import pico_ice_ros2 as ice

ice.run([
    ice.Publish(0x1001, "/pico_ice_button", ice.trigger_if_not_0xff),
    ice.Publish(0x1002, "/pico_ice_ir_remote", ice.trigger_on_interrupt),
    ice.Publish(0x1003, "/pico_ice_bosch_bme280", ice.trigger_every_10ms),
    ice.Subscribe(0x2001, "/imu.orientation.x"),
    ice.Subscribe(0x2002, "/imu.orientation.y"),
    ice.Subscribe(0x2003, "/imu.orientation.z"),
    ice.Subscribe(0x2004, "/imu.orientation.w"),
])
```

This kind of Rosetta Stone would allow ROS2 litteracy to FPGA developers,
and FPGA litteracy to ROS2 developers around a same project.
It is also easier to implement and maintain.


## Sequence diagram

### ROS2 -> ICE40

We would subscribe from ROS2 topics to get messages incoming from the ROS2 network.
Handing of these messages:

```
   ROS2           NavQ+                   RP2040           ICE40 (bus ctrl)    ICE40 (bus peri)
  network   ros2_pico_ice.py            pico_ice.uf2         TopLevel()          BusPeri()
    │              │                        │                   │                   │
    │  -DDS/Zenoh- │                        │                   │                   │
    │ ROS2 message │                        │                   │                   │
    ├─────────────>│                        │                   │                   │
    │              │                        │                   │                   │
    │              │   -Wishbone-Serial-    │                   │                   │
    │              │ice_fpga_serial_bridge()│                   │                   │
    │              │                        │                   │                   │
    │              │   command (1 byte)     │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │   length N (1 byte)    │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │   address (4 bytes)    │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │   value (4*N bytes)    │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │                        │                   │                   │
    │              │                        │  -Wishbone-SPI-   │                   │
    │              │                        │ ice_fpga_write()  │                   │
    │              │                        │                   │                   │
    │              │                        │ command (1 byte)  │                   │
    │              │                        ├──────────────────>│                   │
    │              │                        │ address (4 bytes) │                   │
    │              │                        ├──────────────────>│                   │
    │              │                        │ value (4*N bytes) │                   │
    │              │                        ├──────────────────>│                   │
    │              │                        │                   │                   │
    │              │                        │                   │    Bus valid      │
    │              │                        │                   ├──────────────────>│
    │              │                        │                   │    Bus ready      │
    │              │                        │                   │<──────────────────┤
    │              │                        │                   │                   │
    │              │                        │   ack (1 byte)    │                   │
    │              │                        │<──────────────────┤                   │
    │              │      ack (1 byte)      │                   │                   │
    │              │<───────────────────────┤                   │                   │
```

### ICE40 -> ROS2

Based on rules on the `ros2_pico_ice.py` script (see above), there could also be bus writes
originated by the script, querying the ICE40 and sending the result back.

Below is an example with the ros2_pico_ice.py polling some registers from the FPGA
and sending the value on constant interval. Alternatives such as an interrupt mechanism,
or filtering messages to only send values that are non-null/different than the previous
can also be decided.

```
   ROS2           NavQ+                   RP2040           ICE40 (bus ctrl)    ICE40 (bus peri)
  network   ros2_pico_ice.py            pico_ice.uf2         TopLevel()          BusPeri()
    │              │                        │                   │                   │
    │              │                        │                   │                   │
    │              │                        │                   │                   │
    │              │   -Wishbone-Serial-    │                   │                   │
    │              │ice_fpga_serial_bridge()│                   │                   │
    │              │                        │                   │                   │
    │              │   command (1 byte)     │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │   length N (1 byte)    │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │   address (4 bytes)    │                   │                   │
    │              ├───────────────────────>│                   │                   │
    │              │                        │                   │                   │
    │              │                        │  -Wishbone-SPI-   │                   │
    │              │                        │ ice_fpga_write()  │                   │
    │              │                        │                   │                   │
    │              │                        │ command (1 byte)  │                   │
    │              │                        ├──────────────────>│                   │
    │              │                        │ address (4 bytes) │                   │
    │              │                        ├──────────────────>│                   │
    │              │                        │                   │    Bus valid      │
    │              │                        │                   ├──────────────────>│
    │              │                        │                   │    Bus ready      │
    │              │                        │                   │<──────────────────┤
    │              │                        │   ack (1 byte)    │                   │
    │              │                        │<──────────────────┤                   │
    │              │                        │ value (4*N bytes) │                   │
    │              │                        │<──────────────────┤                   │
    │              │   value (4*N bytes)    │                   │                   │
    │              │<───────────────────────┤                   │                   │
    │              │                        │                   │                   │
    │  -DDS/Zenoh- │                        │                   │                   │
    │ ROS2 message │                        │                   │                   │
    │<─────────────┤                        │                   │                   │
```


### IRQ line

This is the mechanism with which an interrupt signal would go all the way from the FPGA
peripheral to the host.

```
   ROS2           NavQ+                   RP2040           ICE40 (bus ctrl)    ICE40 (bus peri)
  network   ros2_pico_ice.py            pico_ice.uf2         TopLevel()          BusPeri()
    │              │                        │                   │                   │
    │              │                        │                   │      IRQ pin      │
    │              │   -Wishbone-Serial-    │                   │<──────────────────┤
    │              │ice_fpga_serial_bridge()│      IRQ pin      │                   │
    │              │                        │<──────────────────┤                   │
    │              │      irq (1 byte)      │                   │                   │
    │              │<───────────────────────┤                   │                   │
    │              │                        │                   │                   │
    │              │                        │                   │                   │
    :              :                        :                   :                   :

                           ROS2 -> FPGA sequence to read the status register

    :              :                        :                   :                   :
    │              │                        │                   │                   │
    :              :                        :                   :                   :

                         ROS2 -> FPGA sequence to read the peripheral of interest

    :              :                        :                   :                   :
    │              │                        │                   │                   │
    │              │                        │                   │                   │
```

Or, for simplicity:

```
   ROS2           NavQ+                   RP2040           ICE40 (bus ctrl)    ICE40 (bus peri)
  network   ros2_pico_ice.py            pico_ice.uf2         TopLevel()          BusPeri()
    │              │                        │                   │                   │
    │              │                        │                   │      IRQ pin      │
    │              │   -Wishbone-Serial-    │                   │<──────────────────┤
    │              │ice_fpga_serial_bridge()│      IRQ pin      │                   │
    │              │                        │<──────────────────┤                   │
    │              │      irq (1 byte)      │                   │                   │
    │              │<───────────────────────┤                   │                   │
    │              │                        │                   │                   │
    │              │                        │                   │                   │
    :              :                        :                   :                   :

                           ROS2 -> FPGA sequence to read each data register

    :              :                        :                   :                   :
    │              │                        │                   │                   │
    │              │                        │                   │                   │
```


### ROS2 <-> RP2040

It is also possible to make a request stop at the RP2040, using the same diagrams as `ROS2 <-> FPGA` above,
but removing all the RP2040 <-> ICE40 communication.
