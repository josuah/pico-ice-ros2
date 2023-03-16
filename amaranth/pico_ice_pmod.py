import sys
sys.path.insert(0, 'lib/pico-ice-sdk/amaranth')

from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from amaranth_boards.resources import *
#from pmod import *
from pico_ice import *

class PicoIcePmodDemo(Elaboratable):
    def elaborate(self):
        pmod1 = platform.request("pmod", 1)
        pmod2 = platform.request("pmod", 2)
        pmod3 = platform.request("pmod", 3)

        m = Module()
        return m

if __name__ == "__main__":
    platform = PicoIcePlatform()
    platform.add_resources([
        Resource("pmod", 1, Pins("1 2 3 4 7 8 9 10", dir="io", conn=("pmod", 1))),
        Resource("pmod", 2, Pins("1 2 3 4 7 8 9 10", dir="io", conn=("pmod", 2))),
        Resource("pmod", 3, Pins("1 2 3 4 7 8 9 10", dir="io", conn=("pmod", 3))) 
    ])
    platform.build(PicoIcePmodDemo, do_program=True)
