from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from amaranth_boards.resources import *
from pico_ice import PicoIcePlatform

class PicoIcePmodDemo(Elaboratable):
    def elaborate(self):
        spi = platform.request("spi", 1)
        led_r = platform.request("led_r", 0)
        led_g = platform.request("led_g", 0)
        led_b = platform.request("led_b", 0)

        m = Module()

        m.domains += ClockDomain("spi")
        m.d.comb += ClockSignal("spi").eq(spi.clk)

        shift_buf = Signal(8)
        shift_num = Signal(3)
        shift_stb = Signal(1)

        with m.FSM(domain="spi") as fsm:

            with m.State("START"):
                m.d.spi += shift_buf[0].eq(spi.copi)
                m.d.spi += shift_num.eq(7)
                m.next = "RX"

            with m.State("RX"):
                m.d.spi += shift_buf.eq(Cat(shift_buf[1:-1], spi.copi))
                m.d.spi += shift_num.eq(shift_num - 1)
                with m.If(shift_num - 1 == 0):
                    m.d.comb += shift_stb.eq(1)
                    m.next = "START"

        with m.If(~spi.cs):
            m.d.spi += fsm.state.eq(fsm.encoding["START"])

        m.d.comb += spi.cipo.eq(fsm.ongoing("START"))
        m.d.comb += spi.cipo.oe.eq(spi.cs)

        return m

if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([
        SPIResource("spi", 1, clk="21", cipo="26", copi="19", cs_n="25",
                    role="peripheral"),
    ])
    platform.build(PicoIcePmodDemo, do_program=True)
