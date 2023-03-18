from amaranth import *
from amaranth.sim import *


__all__ = [ "NecIrDecoder" ]


class PulseWidthDecoder(Elaboratable):
    """
    Measure the duration between two rising edges of a PWM signal.
    """
    def __init__(self, *, width=8):

        # signal input
        self.rx     = Signal()

        # data output
        self.data   = Signal(width)
        self.done   = Signal()
        self.high   = Signal()

    def elaborate(self, platform):
        m = Module()
        last_rx = Signal(1)

        m.d.slow += last_rx.eq(self.rx)
        m.d.slow += self.data.eq(self.data + 1)

        # on falling edge: send duration of the high period
        with m.If(last_rx & ~self.rx):
            m.d.comb += self.high.eq(1)

        # on rising edge: send duration of the high + low period
        with m.If(~last_rx & self.rx):
            m.d.slow += self.data.eq(0)
            m.d.comb += self.done.eq(1)

        return m


class NecIrDecoder(Elaboratable):
    """
    Decode NEC-style encoded IR signal coming out of an IR sensor.
    https://www.sbprojects.net/knowledge/ir/nec.php
    """

    def __init__(self, *, freq_hz):
        self.freq_hz = freq_hz / 128

        # ir signal input
        self.rx     = Signal()

        # data output
        self.data   = Signal(32)
        self.en     = Signal()
        self.err    = Signal()

    def elaborate(self, platform):
        m = Module()
        sample_num = Signal(range(32 + 1))

        debug = platform.request("debug", 0)

        # New slower clock domain
        m.domains.slow = ClockDomain(local=True)
        clk_div = Signal(8)
        m.d.sync += clk_div.eq(clk_div + 1)
        m.d.comb += ClockSignal("slow").eq(clk_div[-1])

        m.submodules.pwdec = pwdec = PulseWidthDecoder(width=32)
        m.d.comb += pwdec.rx.eq(self.rx)

        def ms_to_ticks(ms):
            return int(self.freq_hz * ms * 1e-3)

        def handle_idle_thres(ms):
            with m.If(pwdec.data > ms_to_ticks(ms)):
                m.d.comb += self.err.eq(1)
                m.next = "IDLE"

        m.d.comb += debug[7].eq(self.rx)
        m.d.comb += debug[6].eq(pwdec.high)
        m.d.comb += debug[5].eq(pwdec.done)
        m.d.comb += debug[4].eq(pwdec.data > ms_to_ticks(1.7))
        m.d.comb += debug[3].eq(pwdec.data > ms_to_ticks(5))
        m.d.comb += debug[2].eq(pwdec.data > ms_to_ticks(7))

        with m.FSM(domain="slow") as fsm:

            with m.State("IDLE"):
                m.d.comb += debug[0].eq(1)
                with m.If(pwdec.high & (pwdec.data > ms_to_ticks(7))):
                    m.next = "PREAMBLE"

            with m.State("PREAMBLE"):
                m.d.comb += debug[1].eq(1)
                handle_idle_thres(20)
                with m.If(pwdec.done):
                    m.next = "SAMPLE"

            with m.State("SAMPLE"):
                handle_idle_thres(5)
                with m.If(pwdec.done):
                    bit = (pwdec.data > ms_to_ticks(1.7))
                    m.d.slow += self.data.eq(Cat(bit, self.data))
                    m.d.slow += sample_num.eq(sample_num + 1)
                with m.If(sample_num == 32):
                    m.d.slow += sample_num.eq(0)
                    m.d.comb += self.en.eq(1)
                    m.next = "IDLE"

        return m

if __name__ == "__main__":
    nul = "                                                     "
    pfx = "################        "
    pfx += "# # # # # # # # #   #   #   #   #   #   # #   "

    rx = nul
    rx += pfx + "# # #   #   # # # # #   #   # # #   #   #   #   #" + nul # Num0
    rx += pfx + "# # # # #   # # # #   #   #   #   # #   #   #   #" + nul # Num1
    rx += pfx + "#   # # # #   # # # # #   #   #   # #   #   #   #" + nul # Num2
    rx += nul

    freq_hz = 2e4 # MHz

    dut = NecIrDecoder(freq_hz=freq_hz)

    def bench():
        for ch in rx:
            for _ in range(int(freq_hz * 562.5e-6)):
                yield
            yield dut.rx.eq(ch == "#")

    sim = Simulator(dut)
    sim.add_clock(1/freq_hz)
    sim.add_slow_process(bench)
    with sim.write_vcd(vcd_file=f"{__file__[:-3]}.vcd"):
        sim.run()
