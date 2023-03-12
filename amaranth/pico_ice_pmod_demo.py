from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from amaranth_boards.resources import *
from pico_ice import PicoIcePlatform

class PicoIcePmodDemo(Elaboratable):
    def elaborate(self):
        m = Module()

        spi = platform.request("spi", 1)
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
        SPIResource("spi", 1, clk="21", cipo="27", copi="19", cs_n="25",
                    role="peripheral"),
    ])
    platform.build(PicoIcePmodDemo, do_program=True)



