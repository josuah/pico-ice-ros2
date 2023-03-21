from binascii import hexlify
from amaranth import *
from amaranth.sim import *
from amaranth.hdl.rec import *


__all__ = [ "SpiBridge", "spi_layout" ]


# soon to be deprecated by Interface()s
def spi_layout(mode):
    assert mode in ("peripheral", "controller")

    controller_driven = { "controller": DIR_FANOUT, "peripheral": DIR_FANIN }
    peripheral_driven = { "peripheral": DIR_FANOUT, "controller": DIR_FANIN }

    return [
        ("clk",    1, controller_driven[mode]),
        ("copi",   1, controller_driven[mode]),
        ("cipo",   1, peripheral_driven[mode]),
        ("cs",     1, controller_driven[mode]),
    ]


class SpiPeripheral(Elaboratable):
    def __init__(self):
        self.spi = Record(spi_layout("peripheral")) # soon to be deprecated by Interface()s
        self.in_en = Signal(1)
        self.in_data = Signal(8)
        self.out_en = Signal(1)
        self.out_data = Signal(8)
        self.beg = Signal(1)
        self.end = Signal(1)

    def elaborate(self, platform):
        m = Module()

        shift_copi = Signal(8)
        shift_cipo = Signal(8)
        next_cipo = Signal(8)
        last_spi_clk = Signal(1)
        last_spi_cs  = Signal(1)
        spi_sampling_edge = Signal(1)
        spi_updating_edge = Signal(1)
        shift_count = Signal(range(8))

        # detect the clock edges
        m.d.sync += last_spi_clk.eq(self.spi.clk)
        m.d.sync += last_spi_cs.eq(self.spi.cs)
        m.d.comb += self.beg.eq(~last_spi_cs & self.spi.cs)
        m.d.comb += self.end.eq(last_spi_cs & ~self.spi.cs)
        m.d.comb += spi_sampling_edge.eq(~last_spi_clk & self.spi.clk)
        m.d.comb += spi_updating_edge.eq(last_spi_clk & ~self.spi.clk)

        # input data in from straight from the shift register
        m.d.comb += self.in_data.eq(shift_copi)

        # shift data out through the shift register
        m.d.comb += self.spi.cipo.eq(shift_cipo[-1])

        m.d.sync += self.in_en.eq(0)
        m.d.comb += self.out_en.eq(self.beg)
        m.d.sync += shift_cipo.eq(shift_cipo)

        with m.If(self.spi.cs):

            with m.If(spi_sampling_edge):
                with m.If(shift_count == 7):
                    m.d.sync += self.in_en.eq(1)
                # The data should be MSB first at the end: insert new data at
                # the LSB and let it shift all the way up to the MSB
                m.d.sync += shift_copi.eq(Cat(self.spi.copi, shift_copi[0:7]))

            with m.If(spi_updating_edge):
                with m.If(shift_count == 7):
                    m.d.comb += self.out_en.eq(1)
                m.d.comb += next_cipo.eq(Cat(C(0), shift_cipo[0:-1]))
                m.d.sync += shift_cipo.eq(next_cipo)
                m.d.comb += self.spi.cipo.eq(next_cipo[-1])
                m.d.sync += shift_count.eq(shift_count + 1)

        with m.Else():
            m.d.sync += shift_cipo.eq(0)
            m.d.sync += shift_copi.eq(0)
            m.d.sync += shift_count.eq(0)

        with m.If(self.out_en):
            m.d.comb += next_cipo.eq(self.out_data)

        return m


if __name__ == "__main__":
    dut = SpiPeripheral()

    clk_freq_hz = 10e6 # MHz
    spi_freq_hz = 2e6  # MHz

    copi = "_____ ##__##__##__##__##__##__##__##__########________####____####__________"
    clk  = "_______#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#______"
    cs   = "_____##################################################################_____"
    cipo = ""
    in_data = bytearray()
    out_data = b"\xFE\x55\x01\x8F\x00\x00"

    def bench():
        global cipo
        global in_data
        global out_data

        o = 0

        for i in range(len(clk)):
            for _ in range(int(clk_freq_hz / spi_freq_hz)):
                yield

                if (yield dut.in_en) == 1:
                    in_data.append((yield dut.in_data))
                if (yield dut.out_en) == 1:
                    o += 1
                yield dut.out_data.eq(out_data[o])

            cipo += "#" if (yield dut.spi.cipo) else "_"

            yield dut.spi.copi  .eq(copi[i] == "#")
            yield dut.spi.clk   .eq(clk[i] == "#")
            yield dut.spi.cs    .eq(cs[i] == "#")

    sim = Simulator(dut)
    sim.add_clock(1 / clk_freq_hz)
    sim.add_sync_process(bench)
    with sim.write_vcd(vcd_file=f"{__file__[:-3]}.vcd"):
        sim.run()

    print(f"cipo={cipo}")
    print(f"data={in_data}")

    assert in_data == b"\xAA\xAA\xF0\xCC"
