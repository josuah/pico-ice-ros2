import sys
sys.path.insert(0, 'lib/pico-ice-sdk/amaranth')
from amaranth import *
from amaranth.build import *
from amaranth_boards.extensions.pmod import *
from amaranth_boards.resources import *
from pico_ice import *
from pmod_7seg import *


class PulseWidthDecoder(Elaboratable):
    """
    Measure the duration of high and low states of a PWM signal.
    """

    def __init__(self, *, rx, width=8):
        self.rx     = rx
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
    def __init__(self, platform, *, rx,
            idle_limit_s=10e-3,
            data_limit_s=(((1.125e-3 + 2.25e-3) / 2)),
            preamble_limit_s=10e-3):
        freq_hz = int(platform.default_clk_frequency)

        self.rx     = rx
        self.data   = Signal(32)
        self.en     = Signal()
        self.err    = Signal()
        self.idle_limit_ticks = int(freq_hz * idle_limit_s)
        self.data_limit_ticks = int(freq_hz * data_limit_s)
        self.preamble_limit_ticks = int(freq_hz * preamble_limit_s)

    def elaborate(self, platform):
        m = Module()
        m.submodules.pwdec = pwdec = PulseWidthDecoder(rx=self.rx, width=32)
        sample_num = Signal(range(32))
        sample_buf = Signal(32)

        led_g = platform.request("led_g", 0)

        m.d.comb += self.data.eq(sample_buf)

        def handle_idle_limit():
            with m.If(pwdec.data > self.idle_limit_ticks):
                m.d.comb += self.err.eq(1)
                m.next = "IDLE"

        with m.FSM() as fsm:

            with m.State("IDLE"):
                with m.If(pwdec.en & (pwdec.data > self.preamble_limit_ticks)):
                    m.next = "PREAMBLE"

            with m.State("PREAMBLE"):
                handle_idle_limit()
                with m.If(pwdec.en):
                    m.d.sync += sample_num.eq(0)
                    m.next = "SAMPLE"

            with m.State("SAMPLE"):
                handle_idle_limit()
                with m.If(pwdec.en):
                    bit = (pwdec.data > self.data_limit_ticks)
                    with m.If(bit):
                        m.d.sync += led_g.eq(1)
                    m.d.sync += sample_buf.eq(Cat(bit, sample_buf))
                    m.d.sync += sample_num.eq(sample_num + 1)
                with m.If(sample_num == 32 - 1):
                    m.d.sync += self.en.eq(1)
                    m.next = "IDLE"

        return m


class PicoIceDemo(Elaboratable):
    def elaborate(self):
        led_r = platform.request("led_r", 0)
        debug = platform.request("debug", 0)
        irsensor = platform.request("irsensor", 0)
        pmod_7seg = platform.request("pmod_7seg", 0)

        m = Module()
        rx_d1 = Signal(1)
        debug_byte = Signal(8)

        p7seg = m.submodules.p7seg = Pmod7Seg(pmod_7seg, debug_byte)
        irdec = m.submodules.irdec = NecIrDecoder(platform, rx=irsensor.rx)

        with m.If(irdec.en):
            m.d.sync += debug_byte.eq(irdec.data[0:8])

        # Visual feedback of IR remote action with an LED
        m.d.comb += led_r.eq(irsensor.rx)
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
    platform.build(PicoIceDemo, do_program=True)
