import sys, os
sys.path.append(os.path.dirname(__file__) + "/../pico-ice-sdk/amaranth")

# amaranth
from amaranth import *
from amaranth.build import *
from amaranth.cli import main
from amaranth_boards.resources import *

# pico-ice-sdk
from pico_ice import *

# pico-ice-pmod
from pmod_7seg import *
from nec_ir_decoder import *
from spi_peripheral import *
from debouncer import *


class TopLevel(Elaboratable):
    """
    Top module to test each Pmod with real hardware.
    """

    def elaborate(self):
        freq_hz = platform.default_clk_frequency
        led = platform.request("led_g", 0)
        irsensor = platform.request("irsensor", 0)
        pmod_7seg = platform.request("pmod_7seg", 0)
        spi = platform.request("spi", 1)

        m = Module()
        irbyte = Signal(8)

        # Debouncer
        m.submodules.debnc = debnc = Debouncer(width=32)
        m.d.comb += debnc.i.eq(irsensor.rx)

        # IR decoder fed into the 7-segment and SPI interface
        m.submodules.irdec = irdec = NecIrDecoder(freq_hz=freq_hz)
        m.d.comb += irdec.rx.eq(debnc.o)

        # 7-segment display there for debugging
        m.submodules.p7seg = p7seg = Pmod7Seg()
        m.d.comb += pmod_7seg.com.eq(p7seg.com)
        m.d.comb += pmod_7seg.seg.eq(p7seg.seg)
        m.d.comb += p7seg.byte.eq(irbyte)

        m.submodules.spi_cs = spi_cs = Debouncer(width=8)
        m.d.comb += spi_cs.i.eq(spi.cs)
        m.submodules.spi_clk = spi_clk = Debouncer(width=8)
        m.d.comb += spi_clk.i.eq(spi.clk)
        m.submodules.spi_copi = spi_copi = Debouncer(width=8)
        m.d.comb += spi_copi.i.eq(spi.copi)

        # SPI peripheral bridge for querying the IR data
        m.submodules.spiperi = spiperi = SPIPeripheral()
        m.d.comb += spi.cipo.oe.eq(1)
        m.d.comb += spi.cipo.o.eq(spiperi.spi.cipo)
        m.d.comb += spiperi.spi.copi.eq(spi_copi.o)
        m.d.comb += spiperi.spi.clk.eq(spi_clk.o)
        m.d.comb += spiperi.spi.cs.eq(spi_cs.o)

        # hookup the SPI peripheral to the IR sensor
        with m.If(spiperi.tx.ready):
            m.d.sync += spiperi.tx.valid.eq(0)
        with m.If(irdec.valid):
            m.d.sync += spiperi.tx.valid.eq(1)
            m.d.sync += spiperi.tx.data.eq(irdec.data[8:])
            m.d.sync += irbyte.eq(irdec.data[8:])

        # visual feedback of IR remote action with an LED
        m.d.comb += led.eq(irsensor.rx)

        return m


if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([

        # The IR sensor only takes 1 signal pin, but it also fits a Pmod
        Resource("irsensor", 0,
            Subsignal("rx", PinsN("4", dir="i", conn=("pmod", 2)))),

        Pmod7SegResource("pmod_7seg", 0, pmod=1),

        SPIResource("spi", 1, cipo="7", copi="10", clk="9", cs_n="8", conn=("pmod", 3), role="peripheral"),

        Resource("debug", 0, Pins("1 2 3 4", conn=("pmod", 3), dir="o")),
    ])
    platform.build(TopLevel, do_program=True, program_opts={"dfu_alt": 1})
