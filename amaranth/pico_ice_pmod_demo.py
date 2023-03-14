from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from amaranth_boards.resources import *
from pico_ice import PicoIcePlatform

class SPIPeripheral(Elaboratable):
    def __init__(self, spi):
        self.shift_buf = Signal(8)
        self.shift_stb = Signal(1)
        self.spi = spi

    def elaborate(self, platform):
        m = Module()
        m.domains += ClockDomain("spi")
        m.d.comb += ClockSignal("spi").eq(self.spi.clk)

        shift_num = Signal(3)

        with m.FSM(domain="spi") as fsm:

            with m.State("START"):
                m.d.spi += self.shift_buf[0].eq(self.spi.copi)
                m.d.spi += shift_num.eq(7)
                m.next = "RX"

            with m.State("RX"):
                m.d.spi += self.shift_buf.eq(Cat(self.shift_buf[1:-1], self.spi.copi))
                m.d.spi += shift_num.eq(shift_num - 1)
                with m.If(shift_num - 1 == 0):
                    m.d.comb += self.shift_stb.eq(1)
                    m.next = "START"

        with m.If(~self.spi.cs):
            m.d.spi += fsm.state.eq(fsm.encoding["START"])

        m.d.comb += self.spi.cipo.eq(fsm.ongoing("START"))
        m.d.comb += self.spi.cipo.oe.eq(self.spi.cs)

        return m

class PicoIcePmodDemo(Elaboratable):
    def elaborate(self):
        spi = platform.request("spi", 1)

        m = Module()

        return SPIPeripheral(spi)

if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([
        SPIResource("spi", 1, clk="21", cipo="27", copi="19", cs_n="25",
                    role="peripheral"),
    ])
    platform.build(PicoIcePmodDemo, do_program=True)
