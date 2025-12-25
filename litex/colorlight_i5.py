#!/usr/bin/env python3

#
# This file is part of LiteX-Boards.
#
# Copyright (c) 2021 Kazumoto Kojima <kkojima@rr.iij4u.or.jp>
# SPDX-License-Identifier: BSD-2-Clause

from migen import *

from litex.gen import *

from litex.build.io import DDROutput

from litex_boards.platforms import colorlight_i5

from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.integration.builder import *
from litex.soc.cores.led import LedChaser

# Imports necessários para o projeto
from litex.soc.cores.spi import SPIMaster
from litex.soc.cores.bitbang import I2CMaster
from litex.soc.cores.gpio import GPIOOut
from litex.build.generic_platform import Subsignal, Pins, IOStandard

from litedram.modules import M12L64322A # Compatible with EM638325-6H.
from litedram.modules import IS42S16160
from litedram.phy import GENSDRPHY, HalfRateGENSDRPHY

# CRG (Clock and Reset Generation) -----------------------------------------------------------------

class _CRG(LiteXModule):
    def __init__(self, platform, sys_clk_freq, sdram_rate="1:1"):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()
        if sdram_rate == "1:2":
            self.cd_sys2x    = ClockDomain()
            self.cd_sys2x_ps = ClockDomain()
        else:
            self.cd_sys_ps = ClockDomain()

        # # #

        # Clk / Rst
        # Usa o oscilador externo de 25MHz
        clk = platform.request("clk25")
        clk_freq = 25e6

        rst_n = platform.request("cpu_reset_n")

        # PLL Principal
        self.pll = pll = ECP5PLL()
        self.comb += pll.reset.eq(~rst_n | self.rst)
        pll.register_clkin(clk, clk_freq)
        pll.create_clkout(self.cd_sys,    sys_clk_freq)
        if sdram_rate == "1:2":
            pll.create_clkout(self.cd_sys2x,    2*sys_clk_freq)
            pll.create_clkout(self.cd_sys2x_ps, 2*sys_clk_freq, phase=180)
        else:
           pll.create_clkout(self.cd_sys_ps, sys_clk_freq, phase=180)

        # SDRAM clock
        sdram_clk = ClockSignal("sys2x_ps" if sdram_rate == "1:2" else "sys_ps")
        self.specials += DDROutput(1, 0, platform.request("sdram_clock"), sdram_clk)

# BaseSoC ------------------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    def __init__(self, board="i5", revision="7.0", toolchain="trellis", sys_clk_freq=60e6,
        sdram_rate             = "1:1",
        with_led_chaser        = True,
        **kwargs):
        board = board.lower()
        assert board in ["i5", "i9"]
        platform = colorlight_i5.Platform(board=board, revision=revision, toolchain=toolchain)

        # CRG --------------------------------------------------------------------------------------
        self.crg = _CRG(platform, sys_clk_freq,
            sdram_rate       = sdram_rate
        )

        # SoCCore ----------------------------------------------------------------------------------

        # Remove parâmetros que NÃO podem ser sobrescritos pelo parser
        kwargs.pop("integrated_rom_size", None)
        kwargs.pop("integrated_sram_size", None)

        SoCCore.__init__(
            self,
            platform,
            int(sys_clk_freq),
            ident="LiteX SoC on Colorlight " + board.upper(),
            integrated_rom_size=0x20000,
            integrated_sram_size=0x8000,
            integrated_main_ram_size=0,
            with_uartboot=False,              #  correto
            cpu_reset_address=0x00200000,     #  ESSENCIAL
            **kwargs
        )


        # Leds -------------------------------------------------------------------------------------
        if with_led_chaser:
            ledn = platform.request_all("user_led_n")
            self.leds = LedChaser(pads=ledn, sys_clk_freq=sys_clk_freq)

        # SPI Flash --------------------------------------------------------------------------------
        if board == "i5":
            from litespi.modules import GD25Q16 as SpiFlashModule
        if board == "i9":
            from litespi.modules import W25Q64 as SpiFlashModule

        from litespi.opcodes import SpiNorFlashOpCodes as Codes
        self.add_spi_flash(mode="1x", clk_freq=30e6, module=SpiFlashModule(Codes.READ_1_1_1), rate="1:1", boot=True)
        self.add_constant("FLASH_BOOT_ADDRESS", 0x00200000)

        # SDR SDRAM --------------------------------------------------------------------------------
        if not self.integrated_main_ram_size:
            sdrphy_cls = HalfRateGENSDRPHY if sdram_rate == "1:2" else GENSDRPHY
            self.sdrphy = sdrphy_cls(platform.request("sdram"))
            self.add_sdram("sdram",
                phy           = self.sdrphy,
                module        = M12L64322A(sys_clk_freq, sdram_rate),
                l2_cache_size = kwargs.get("l2_size", 8192)
            )

        # Configuração dos pinos SPI (para display) no conector CN2 -----------------------------
        spi_pads = [
            ("spi", 0,
                Subsignal("clk",  Pins("G20")),
                Subsignal("mosi", Pins("L18")),
                Subsignal("miso", Pins("M18")),   # <- opcional p/ ST7789; pode ficar desconectado no LCD
                Subsignal("cs_n", Pins("N17")),
                IOStandard("LVCMOS33")
            ),
            # Pino DC do display
            ("lcd_dc", 0, Pins("N18"), IOStandard("LVCMOS33")),

            # Pino RESET do display
            ("lcd_reset", 0, Pins("L20"), IOStandard("LVCMOS33")),

            # Pino BACKLIGHT
            ("lcd_blk", 0, Pins("P18"), IOStandard("LVCMOS33")),
        ]

        platform.add_extension(spi_pads)

        # Adiciona o Core SPI Master e o CSR 'spi'
        self.spi = SPIMaster( pads = platform.request("spi"), data_width = 8, sys_clk_freq = sys_clk_freq,spi_clk_freq = 40e6)
        self.add_csr("spi")


        # Adiciona o Core GPIOOut e o CSR 'lcd_reset'
        self.submodules.lcd_dc = GPIOOut(platform.request("lcd_dc"))
        self.add_csr("lcd_dc")

        self.submodules.lcd_reset = GPIOOut(platform.request("lcd_reset"))
        self.add_csr("lcd_reset")

        self.submodules.lcd_blk = GPIOOut(platform.request("lcd_blk"))
        self.add_csr("lcd_blk")

        # Configuração dos pinos I2C no conector J2 -----------------------------------
        i2c_pads = [
            ("i2c", 0,
                Subsignal("scl", Pins("U17")),
                Subsignal("sda", Pins("U18")),
                IOStandard("LVCMOS33")
            )
        ]

        platform.add_extension(i2c_pads)
        
        # Adiciona o Core I2CMaster (Bitbang) e o CSR 'i2c'
        self.submodules.i2c = I2CMaster(pads=platform.request("i2c"))
        self.add_csr("i2c")

# Build --------------------------------------------------------------------------------------------

def main():
    from litex.build.parser import LiteXArgumentParser
    parser = LiteXArgumentParser(platform=colorlight_i5.Platform, description="LiteX SoC on Colorlight I5.")
    parser.add_target_argument("--board",            default="i5",             help="Board type (i5).")
    parser.add_target_argument("--revision",         default="7.0",            help="Board revision (7.0).")
    parser.add_target_argument("--sys-clk-freq",     default=60e6, type=float, help="System clock frequency.")
    parser.add_target_argument("--sdram-rate",       default="1:1",            help="SDRAM Rate (1:1 Full Rate or 1:2 Half Rate).")
    
    
    args = parser.parse_args()

    soc = BaseSoC(board=args.board, revision=args.revision,
        toolchain              = args.toolchain,
        sys_clk_freq           = args.sys_clk_freq,
        sdram_rate             = args.sdram_rate,
        **parser.soc_argdict
    )

    builder = Builder(soc, **parser.builder_argdict)
    if args.build:
        builder.build(**parser.toolchain_argdict)

    if args.load:
        prog = soc.platform.create_programmer()
        prog.load_bitstream(builder.get_bitstream_filename(mode="sram"))

if __name__ == "__main__":
    main()