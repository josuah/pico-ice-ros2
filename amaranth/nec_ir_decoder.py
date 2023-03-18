from amaranth import *
from amaranth.sim import *

class PulseWidthDecoder(Elaboratable):
    """
    Measure the duration of high and low states of a PWM signal.
    """
    def __init__(self, *, width=8):

        # signal input
        self.rx     = Signal()

        # data output
        self.data   = Signal(width)
        self.en     = Signal()

    def elaborate(self, platform):
        m = Module()
        last_rx = Signal(1)
        cycles  = Signal(self.data.shape())

        m.d.sync += last_rx.eq(self.rx)
        m.d.comb += self.data.eq(cycles)

        # on falling edge
        with m.If(last_rx & ~self.rx):
            m.d.sync += cycles.eq(0)
            m.d.comb += self.en.eq(1)
        with m.Else():
            m.d.sync += cycles.eq(cycles + 1)

        return m


class NecIrDecoder(Elaboratable):
    """
    Decode NEC-style encoded IR signal coming out of an IR sensor.
    https://www.sbprojects.net/knowledge/ir/nec.php
    """

    def __init__(self, *, freq_hz):
        self.freq_hz = freq_hz

        # ir signal input
        self.rx     = Signal()

        # data output
        self.data   = Signal(32)
        self.en     = Signal()
        self.err    = Signal()

    def elaborate(self, platform):
        m = Module()
        sample_num = Signal(range(32))
        sample_buf = Signal(32)

        m.submodules.pwdec = pwdec = PulseWidthDecoder(width=32)
        m.d.comb += pwdec.rx.eq(self.rx)

        idle_thres_ticks   = int(self.freq_hz * 1e-3)
        sample_thres_ticks = int(self.freq_hz * (1.125e-3 + 2.25e-3) / 2)
        prefix_thres_ticks = int(self.freq_hz * 1e-3)

        m.d.comb += self.data.eq(sample_buf)

        def handle_idle_thres():
            with m.If(pwdec.data > idle_thres_ticks):
                m.d.comb += self.err.eq(1)
                m.next = "IDLE"

        with m.FSM() as fsm:

            with m.State("IDLE"):
                with m.If(pwdec.en & (pwdec.data > prefix_thres_ticks)):
                    m.next = "PREFIX"

            with m.State("PREFIX"):
                handle_idle_thres()
                with m.If(pwdec.en):
                    m.d.sync += sample_num.eq(0)
                    m.next = "SAMPLE"

            with m.State("SAMPLE"):
                handle_idle_thres()
                with m.If(pwdec.en):
                    bit = (pwdec.data > sample_thres_ticks)
                    m.d.sync += sample_buf.eq(Cat(bit, sample_buf))
                    m.d.sync += sample_num.eq(sample_num + 1)
                with m.If(sample_num == 32 - 1):
                    m.d.sync += self.en.eq(1)
                    m.next = "IDLE"

        return m


def test(name, stream):
    freq_hz = 2e4 # MHz

    dut = NecIrDecoder(freq_hz=freq_hz)

    def bench():
        for ch in stream:
            print(ch)
            for _ in range(int(freq_hz * 562.5e-6)):
                yield
            yield dut.rx.eq(ch == "#")

    sim = Simulator(dut)
    sim.add_clock(1/freq_hz)
    sim.add_sync_process(bench)
    with sim.write_vcd("nec_ir_decoder.vcd"):
        sim.run()


if __name__ == "__main__":

    rx = ""
    rx += "################        "
    rx += "# # # # # # # ### ### ### ### ### ### # ### "

    test("0",     rx + "# # ### ### # # # # ### ### # # ### ### ### ###")
    test("1",     rx + "# # # # ### # # # ### ### ### ### # ### ### ###")
    test("2",     rx + "### # # # ### # # # # ### ### ### # ### ### ###")
    test("3",     rx + "# ### # # ### # # # ### # ### ### # ### ### ###")
    test("4",     rx + "# # ### # ### # # # ### ### # ### # ### ### ###")
    test("5",     rx + "### # ### # ### # # # # ### # ### # ### ### ###")
    test("6",     rx + "# ### ### # ### # # # ### # # ### # ### ### ###")
    test("7",     rx + "# # # ### ### # # # ### ### ### # # ### ### ###")
    test("8",     rx + "### # # ### ### # # # ### ### # # # ### ### ###")
    test("9",     rx + "# ### # ### ### # # # ### # ### # # ### ### ###")
    test("back",  rx + "# ### ### ### # # # # ### # # # ### ### ### ###")
    test("ch+",   rx + "# ### # ### # # # # ### # ### # ### ### ### ###")
    test("ch-",   rx + "# # # ### # # # # ### ### ### # ### ### ### ###")
    test("enter", rx + "### # # ### # # # # # ### # ### ### ### ### ###")
    test("next",  rx + "### # ### ### # # # # # ### # # ### ### ### ###")
    test("play",  rx + "### # # # # # # # # ### ### ### ### ### ### ###")
    test("prev",  rx + "# ### ### # ### # # # ### # # ### # ### ### ###")
    test("setup", rx + "# # ### # # # # # ### ### # ### ### ### ### ###")
    test("stop",  rx + "# ### # ### ### # # # ### # ### # # ### ### ###")
    test("vol+",  rx + "# ### # # # # # # ### # ### ### ### ### ### ###")
    test("vol-",  rx + "# # # # # # # # ### ### ### ### ### ### ### ###")
