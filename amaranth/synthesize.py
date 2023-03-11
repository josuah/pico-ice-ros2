from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from pico_ice import PicoIcePlatform

class PinBlinker(Elaboratable):
    def __init__(self):
        timer = Signal(unsigned(20))

    def elaborate(self):
        m = Module()

        spi = platform.request("spi", 0)
        led_r = platform.request("led_r", 0)
        led_g = platform.request("led_g", 0)
        led_b = platform.request("led_b", 0)

        m.d.comb += led_r.o.eq(spi.clk)
        m.d.comb += led_g.o.eq(spi.cs)
        m.d.comb += led_b.o.eq(spi.copi)

        return m

if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([
        PmodGPIOType1Resource("gpio", 0, pmod=0),
    ])
    platform.build(PinBlinker, debug_verilog=True)
