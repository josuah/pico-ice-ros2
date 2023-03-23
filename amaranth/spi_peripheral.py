from binascii import hexlify
from amaranth import *
from amaranth.sim import *
from amaranth.hdl.rec import *
from debouncer import *


__all__ = [ "SPIPeripheral" ]


class ValidReadyInterface(Record):
    def __init__(self, *, width):
        super().__init__([
            ("data",     width, DIR_FANOUT),
            ("valid",    1,     DIR_FANOUT),
            ("ready",    1,     DIR_FANIN),
        ])


class SPIInterface(Record):
    def __init__(self, *, mode):
        assert mode in ("peripheral", "controller")

        controller_driven = { "controller": DIR_FANOUT, "peripheral": DIR_FANIN }
        peripheral_driven = { "peripheral": DIR_FANOUT, "controller": DIR_FANIN }

        return super().__init__([
            ("clk",    1, controller_driven[mode]),
            ("copi",   1, controller_driven[mode]),
            ("cipo",   1, peripheral_driven[mode]),
            ("cs",     1, controller_driven[mode]),
        ])


class SPIPeripheral(Elaboratable):
    """
    SPI Peripheral with debounced inputs.
    """

    def __init__(self, *, debounce=8):
        self.debounce = debounce

        # Record()s are soon to be deprecated by Interface()s
        self.spi    = SPIInterface(mode="peripheral")
        self.tx     = ValidReadyInterface(width=8)
        self.rx     = ValidReadyInterface(width=8)
        self.beg    = Signal(1)
        self.end    = Signal(1)

    def elaborate(self, platform):
        m = Module()
        shift_copi      = Signal(8)
        shift_cipo      = Signal(8)
        next_cipo       = Signal(8)
        next_copi       = Signal(8)
        last_clk        = Signal(1)
        last_cs         = Signal(1)
        sampling_edge   = Signal(1)
        updating_edge   = Signal(1)
        shift_count     = Signal(range(8))
        reload_rx       = Signal(1)
        reload_tx       = Signal(1)

        # detect the clock edges
        m.d.sync += last_clk.eq(self.spi.clk)
        m.d.sync += last_cs.eq(self.spi.cs)
        m.d.comb += self.beg.eq(~last_cs & self.spi.cs)
        m.d.comb += self.end.eq(last_cs & ~self.spi.cs)
        m.d.comb += sampling_edge.eq(~last_clk & self.spi.clk)
        m.d.comb += updating_edge.eq(last_clk & ~self.spi.clk)

        # bind shift registers to the I/O ports
        m.d.comb += self.spi.cipo.eq(shift_cipo[-1])

        self.tx.ready.reset = 1

        # exchange data with the internal module
        with m.If(self.tx.ready & self.tx.valid):
            m.d.sync += next_cipo.eq(self.tx.data)
            m.d.sync += self.tx.ready.eq(0)
        with m.If(self.rx.ready & self.rx.valid):
            m.d.sync += self.rx.valid.eq(0)

        # shift data in and out on clock edges
        m.d.sync += reload_rx.eq(0)
        m.d.sync += reload_tx.eq(0)
        with m.If(self.spi.cs):
            with m.If(updating_edge):
                m.d.sync += shift_cipo.eq(Cat(0, shift_cipo))
                with m.If(shift_count == 0):
                    m.d.sync += reload_tx.eq(1)
            with m.If(sampling_edge):
                m.d.sync += shift_copi.eq(Cat(self.spi.copi, shift_copi[0:7]))
                m.d.sync += shift_count.eq(shift_count + 1)
                with m.If(shift_count == 7):
                    m.d.sync += reload_rx.eq(1)

        # reload the shift registers with the next data
        with m.If(reload_rx):
            m.d.sync += self.rx.valid.eq(1)
            m.d.sync += self.rx.data.eq(shift_copi)
        with m.If(reload_tx):
            m.d.sync += self.tx.ready.eq(1)
            m.d.sync += shift_cipo.eq(next_cipo)
            m.d.sync += next_cipo.eq(0)

        return m


if __name__ == "__main__":
    dut = SPIPeripheral()

    clk_freq_hz = 10e6 # MHz
    spi_freq_hz = 2e6  # MHz

    copi = "_____ ##__##__##__##__##__##__##__##__########________####____####__________"
    clk  = "_______#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#_#______"
    cs   = "_____##################################################################_____"
    cipo = ""
    rx_data = bytearray()
    tx_data = b"\xFE\x55\x01\x8F\x00\x00\x00\x00"

    def bench():
        global cipo
        global rx_data
        global tx_data

        n = 0

        yield dut.rx.ready.eq(1)

        for i in range(len(clk)):
            for _ in range(int(clk_freq_hz / spi_freq_hz)):
                yield

                yield dut.tx.data.eq(tx_data[n])
                yield dut.tx.valid.eq(1)
                if (yield dut.tx.ready) == 1:
                    n += 1
                if (yield dut.rx.valid) == 1:
                    rx_data.append((yield dut.rx.data))

            cipo += "#" if (yield dut.spi.cipo) else "_"

            yield dut.spi.copi.eq(copi[i] == "#")
            yield dut.spi.clk.eq(clk[i] == "#")
            yield dut.spi.cs.eq(cs[i] == "#")

    sim = Simulator(dut)
    sim.add_clock(1 / clk_freq_hz)
    sim.add_sync_process(bench)
    with sim.write_vcd(vcd_file=f"{__file__[:-3]}.vcd"):
        sim.run()

    print(f"cipo={cipo}")
    print(f"copi={rx_data}")

    assert rx_data == b"\xAA\xAA\xF0\xCC"
