from amaranth.build import *
from amaranth.vendor.lattice_ice40 import *
from amaranth_boards.resources import *


__all__ = ["PicoIcePlatform"]


class PicoIcePlatform(LatticeICE40Platform):
    device      = "iCE40UP5K"
    package     = "SG48"
    default_clk = "SB_HFOSC"
    hfosc_div   = 0
    resources   = [
        *LEDResources(pins="39 40 41",
            invert=True, attrs=Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("led_g", 0,
            PinsN("39", dir="o"), Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("led_b", 0,
            PinsN("40", dir="o"), Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("led_r", 0,
            PinsN("41", dir="o"), Attrs(IO_STANDARD="SB_LVCMOS")),
        *SPIFlashResources(0,
            cs_n="16", clk="15", copi="14", cipo="17",
            attrs=Attrs(IO_STANDARD="SB_LVCMOS")),
        SPIResource("spi", 0,
            cs_n="16", clk="15", copi="14", cipo="17",
            attrs=Attrs(IO_STANDARD="SB_LVCMOS"), role="peripheral"),
    ]
    connectors  = [
        Connector("pmod", 0, "4  2  47 45 - - 3  48 46 44 - -"),
        Connector("pmod", 1, "43 38 34 31 - - 42 36 32 28 - -"),
        Connector("pmod", 2, "27 25 21 19 - - 26 23 20 18 - -"),
    ]
