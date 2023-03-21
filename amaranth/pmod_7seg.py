# https://1bitsquared.com/collections/fpga/products/pmod-7-segment-display

from amaranth import *
from amaranth.build import *


__all__ = [
    "Pmod7SegResource",
    "Pmod7Seg",
]


def Pmod7SegResource(name, number, *args, pmod, common="anode"):
    assert common in ("anode", "cathode")
    return Resource(name, number,
        Subsignal("seg", PinsN("1 2 3 4 7 8 9", dir="o", conn=("pmod", pmod))),
        Subsignal("com",  Pins("10", dir="o", conn=("pmod", pmod))),
        *args
    )


class Pmod7Seg(Elaboratable):
    def __init__(self, *, clk_div=4):
        self.clk_div = clk_div
        self.byte = Signal(8)
        self.seg = Signal(7)
        self.com = Signal(1)

    def elaborate(self, platform):
        m = Module()

        cnt = Signal(self.clk_div)
        seg0 = Signal(7)
        seg1 = Signal(7)

        m.d.sync += cnt.eq(cnt + 1)

        for seg, hex in ((seg0, self.byte[0:4]), (seg1, self.byte[4:8])):
            with m.Switch(hex):
                with m.Case(0x0):
                    m.d.comb += seg.eq(0b0111111)
                with m.Case(0x1):
                    m.d.comb += seg.eq(0b0000110)
                with m.Case(0x2):
                    m.d.comb += seg.eq(0b1011011)
                with m.Case(0x3):
                    m.d.comb += seg.eq(0b1001111)
                with m.Case(0x4):
                    m.d.comb += seg.eq(0b1100110)
                with m.Case(0x5):
                    m.d.comb += seg.eq(0b1101101)
                with m.Case(0x6):
                    m.d.comb += seg.eq(0b1111101)
                with m.Case(0x7):
                    m.d.comb += seg.eq(0b0000111)
                with m.Case(0x8):
                    m.d.comb += seg.eq(0b1111111)
                with m.Case(0x9):
                    m.d.comb += seg.eq(0b1101111)
                with m.Case(0xA):
                    m.d.comb += seg.eq(0b1110111)
                with m.Case(0xB):
                    m.d.comb += seg.eq(0b1111100)
                with m.Case(0xC):
                    m.d.comb += seg.eq(0b0111001)
                with m.Case(0xD):
                    m.d.comb += seg.eq(0b1011110)
                with m.Case(0xE):
                    m.d.comb += seg.eq(0b1111001)
                with m.Case(0xF):
                    m.d.comb += seg.eq(0b1110001)

        with m.If(self.com):
            m.d.comb += self.seg.eq(seg0)
        with m.Else():
            m.d.comb += self.seg.eq(seg1)

        m.d.comb += self.com.eq(cnt[-1])

        return m
