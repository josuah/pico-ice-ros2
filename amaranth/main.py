import sys
import os
sys.path.append(os.path.dirname(__file__) + "/../lib/pico-ice-sdk/amaranth")

# from amaranth
from amaranth import *
from amaranth.build import *
from amaranth.cli import main
from amaranth_boards.resources import *

# from pico-ice-sdk
from pico_ice import *

# from pico-ice-pmod
from pmod_7seg import *
from nec_ir_decoder import *
from spi_peripheral import *


class TopLevel(Elaboratable):
    """
    Top module to test each Pmod with real hardware.
    """

    def elaborate(self):
        freq_hz = platform.default_clk_frequency
        led = platform.request("led_g", 0)
        irsensor = platform.request("irsensor", 0)
        pmod_7seg = platform.request("pmod_7seg", 0)
        #spi = platform.request("spi")
        debug = platform.request("debug")

        m = Module()
        p7seg_byte = Signal(8)

        # IR decoder fed into the 
        m.submodules.irdec = irdec = NecIrDecoder(freq_hz=freq_hz)
        m.d.comb += irdec.rx.eq(irsensor.rx)

        # 7-segment display there for debugging
        m.submodules.p7seg = p7seg = Pmod7Seg()
        m.d.comb += pmod_7seg.com.eq(p7seg.com)
        m.d.comb += pmod_7seg.seg.eq(p7seg.seg)
        m.d.comb += p7seg.byte.eq(p7seg_byte)

        # SPI peripheral bridge for querying the IR data
        m.submodules.spip = spip = SPIPeripheral()
        #m.d.comb += m.spi.eq()

        with m.If(irdec.en):
            m.d.sync += p7seg_byte.eq(irdec.data[8:16])

        # Visual feedback of IR remote action with an LED
        m.d.comb += led.eq(irsensor.rx)

        return m


if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([

        # 8-channel logic analyzer plugged directly onto a Pmod
        Resource("debug", 0,
            Pins("4 10 3 9 2 8 1 7", dir="o", conn=("pmod", 3))),

        # The IR sensor only takes 1 signal pin, but it also fits a Pmod
        Resource("irsensor", 0,
            Subsignal("rx", PinsN("4", dir="i", conn=("pmod", 2)))),

        Pmod7SegResource("pmod_7seg", 0, pmod=1),

#        SPIResource("spi", 0, conn=("pmod", 3))
    ])
    platform.build(TopLevel, do_program=True, program_opts={"dfu_alt": 0})
