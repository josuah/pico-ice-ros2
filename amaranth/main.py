import sys
import os
sys.path.append(os.path.dirname(__file__) + "/../lib/pico-ice-sdk/amaranth")

# amaranth libraries
from amaranth import *
from amaranth.build import *
from amaranth.cli import main

# local libraries
from pmod_7seg import *
from nec_ir_decoder import *
from pico_ice import *


class PicoIceBoard(Elaboratable):
    """
    Top module to test each Pmod with real hardware.
    """

    def elaborate(self):
        freq_hz = platform.default_clk_frequency
        led = platform.request("led_g", 0)
        irsensor = platform.request("irsensor", 0)
        pmod_7seg = platform.request("pmod_7seg", 0)

        div = Signal(8)

        m = Module()

        debug_byte = Signal(8)

        m.submodules.p7seg = p7seg = Pmod7Seg(pmod_7seg, debug_byte)

        m.submodules.irdec = irdec = NecIrDecoder(freq_hz=freq_hz)
        m.d.comb += irdec.rx.eq(irsensor.rx)

        with m.If(irdec.en):
            m.d.sync += debug_byte.eq(irdec.data[8:16])

        # Visual feedback of IR remote action with an LED
        m.d.comb += led.eq(irsensor.rx)

        return m


if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([

        Resource("debug", 0,
            Pins("4 10 3 9 2 8 1 7", dir="o", conn=("pmod", 3))),

        Resource("irsensor", 0,
            Subsignal("rx", PinsN("4", dir="i", conn=("pmod", 2)))),

        Pmod7SegResource("pmod_7seg", 0, pmod=1)
    ])
    platform.build(PicoIceBoard, do_program=True)
