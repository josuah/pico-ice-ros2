import sys
sys.path.insert(0, 'lib/pico-ice-sdk/amaranth')
from amaranth import *
from amaranth.build import *
from amaranth.cli import main
from pico_ice import *
from pmod_7seg import *


class PicoIceBoard(Elaboratable):

    def elaborate(self, platform):
        led = platform.request("led_g", 0)
        debug = platform.request("debug", 0)
        irsensor = platform.request("irsensor", 0)
        pmod_7seg = platform.request("pmod_7seg", 0)

        m = Module()
        rx_d1 = Signal(1)
        debug_byte = Signal(8)

        p7seg = m.submodules.p7seg = Pmod7Seg(pmod_7seg, debug_byte)
        irdec = m.submodules.irdec = NecIrDecoder(rx=irsensor.rx)

        with m.If(irdec.en):
            m.d.sync += debug_byte.eq(irdec.data[0:8])

        # Visual feedback of IR remote action with an LED
        m.d.comb += led.eq(irsensor.rx)
        m.d.comb += debug.o.eq(irsensor.rx)

        return m


if __name__ == "__main__":

    platform = PicoIcePlatform()
    platform.add_resources([

        Resource("debug", 0,
            Pins("11", dir="o")),

        Resource("irsensor", 0,
            Subsignal("rx", PinsN("4", dir="i", conn=("pmod", 2)))),

        Pmod7SegResource("pmod_7seg", 0, pmod=1)
    ])
    platform.build(PicoIceBoard, do_program=True)
