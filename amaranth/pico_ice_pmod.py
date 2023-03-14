from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from amaranth_boards.resources import *
from pico_ice import PicoIcePlatform

class SPIPeripheral(Elaboratable):
    def __init__(self, spi, size=8):
        self.size = size
        self.r_data = Signal(size)
        self.r_en = Signal(1)
        self.spi = spi

    def elaborate(self, platform):
        m = Module()

        shift_num = Signal(range(0, self.size))
        shift_num_next = Signal(shift_num.shape())
        clk_last = Signal(1)
        clk_edge = Signal(1)

        m.d.sync += clk_last.eq(self.spi.clk)
        m.d.comb += clk_edge.eq((clk_last != self.spi.clk) & self.spi.clk)

        with m.If(self.spi.cs):
            with m.If(clk_edge):

                with m.FSM() as fsm:

                    with m.State("START"):
                        m.d.sync += self.r_data[0].eq(self.spi.copi)
                        m.d.sync += shift_num.eq(self.size - 1)
                        m.next = "RX"

                    with m.State("RX"):
                        m.d.sync += self.r_data.eq(Cat(self.r_data[1:-1], self.spi.copi))
                        m.d.sync += shift_num.eq(shift_num - 1)
                        with m.If(shift_num - 1 == 0):
                            m.d.comb += self.r_en.eq(1)
                            m.next = "START"

        with m.Else():
            m.d.sync += fsm.state.eq(fsm.encoding["START"])

        m.d.comb += self.spi.cipo.eq(self.r_en)
        m.d.comb += self.spi.cipo.oe.eq(1)

        return m

class PicoIcePmodDemo(Elaboratable):
    def elaborate(self):
        spi = platform.request("spi", 1)
        led_r = platform.request("led_r", 0)
        led_g = platform.request("led_g", 0)
        led_b = platform.request("led_b", 0)

        m = Module()
        m_spi = m.submodules.spi = SPIPeripheral(spi)

        count = Signal(3)

        with m.If(m_spi.r_en & (m_spi.r_data == 0x0F)):
            m.d.sync += count.eq(count + 1)

        m.d.comb += led_r.eq(count[0])
        m.d.comb += led_g.eq(count[1])
        m.d.comb += led_b.eq(count[2])

        return m

if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([
        SPIResource("spi", 1, clk="21", cipo="27", copi="19", cs_n="25",
                    role="peripheral"),
    ])
    platform.build(PicoIcePmodDemo, do_program=True)
