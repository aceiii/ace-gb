# Ace-GB Emulator

My first GameBoy emulator!!

## Build

./configure.sh
cmake --build build

## Run

./build/ace-gb

## Tests

### Mooneye Test Suite

#### Acceptance

- [x] bits/mem_oam.gb
- [x] bits/reg_f.gb
- [x] bits/unused_hwio-GS.gb
- [x] instr/daa.gb
- [ ] interrupts/ie_push.gb
- [x] oam_dma/basic.gb
- [x] oam_dma/reg_read.gb
- [ ] oam_dma/sources-GS.gb
- [ ] ppu/hblank_ly_scx_timing-GS.gb
- [ ] ppu/intr_1_2_timing-GS.gb
- [ ] ppu/intr_2_0_timing.gb
- [ ] ppu/intr_2_mode0_timing_sprites.gb
- [ ] ppu/intr_2_mode0_timing.gb
- [ ] ppu/intr_2_mode3_timing.gb
- [ ] ppu/intr_2_oam_ok_timing.gb
- [ ] ppu/lcdon_timing-GS.gb
- [ ] ppu/lcdon_write_timing-GS.gb
- [ ] ppu/stat_irq_blocking.gb
- [ ] ppu/stat_lyc_onoff.gb
- [ ] ppu/vblank_stat_intr-GS.gb
- [ ] serial/boot_sclk_align-dmgABCmgb.gb
- [ ] timer/div_write.gb
- [ ] timer/rapid_toggle.gb
- [x] timer/tim00_div_trigger.gb
- [ ] timer/tim00.gb
- [ ] timer/tim01_div_trigger.gb
- [x] timer/tim01.gb
- [ ] timer/tim10_div_trigger.gb
- [ ] timer/tim10.gb
- [x] timer/tim11_div_trigger.gb
- [ ] timer/tim11.gb
- [ ] timer/tima_reload.gb
- [ ] timer/tima_write_reloading.gb
- [ ] timer/tma_write_reloading.gb
- [ ] add_sp_e_timing.gb
- [ ] boot_div-dmg0.gb
- [ ] boot_div-dmgABCmgb.gb
- [ ] boot_div-S.gb
- [ ] boot_div2-S.gb
- [ ] boot_hwio-dmg0.gb
- [ ] boot_hwio-dmgABCmgb.gb
- [ ] boot_hwio-S.gb
- [ ] boot_regs-dmg0.gb
- [x] boot_regs-dmgABC.gb
- [ ] boot_regs-mgb.gb
- [ ] boot_regs-sgb.gb
- [ ] boot_regs-sgb2.gb
- [ ] call_cc_timing.gb
- [ ] call_cc_timing2.gb
- [ ] call_timing.gb
- [ ] call_timing2.gb
- [ ] di_timing-GS.gb
- [x] div_timing.gb
- [ ] ei_sequence.gb
- [ ] ei_timing.gb
- [x] halt_ime0_ei.gb
- [ ] halt_ime0_nointr_timing.gb
- [ ] halt_ime1_timing.gb
- [ ] halt_ime1_timing2-GS.gb
- [x] if_ie_registers.gb
- [x] intr_timing.gb
- [ ] jp_cc_timing.gb
- [ ] jp_timing.gb
- [ ] ld_hl_sp_e_timing.gb
- [ ] oam_dma_restart.gb
- [ ] oam_dma_start.gb
- [ ] oam_dma_timing.gb
- [x] pop_timing.gb
- [ ] push_timing.gb
- [ ] rapid_di_ei.gb
- [ ] ret_cc_timing.gb
- [ ] ret_timing.gb
- [ ] reti_intr_timing.gb
- [ ] reti_timing.gb
- [ ] rst_timing.gb

#### Emulator only

- [ ] mbc1/bits_bank1.gb
- [ ] mbc1/bits_bank2.gb
- [ ] mbc1/bits_mode.gb
- [ ] mbc1/bits_ramg.gb
- [ ] mbc1/multicart_rom_8Mb.gb
- [ ] mbc1/ram_256kb.gb
- [ ] mbc1/ram_64kb.gb
- [ ] mbc1/rom_16Mb.gb
- [ ] mbc1/rom_1Mb.gb
- [ ] mbc1/rom_2Mb.gb
- [ ] mbc1/rom_4Mb.gb
- [ ] mbc1/rom_512kb.gb
- [ ] mbc1/rom_8Mb.gb
- [ ] mbc2/bits_ramg.gb
- [ ] mbc2/bits_romb.gb
- [ ] mbc2/bits_unused.gb
- [ ] mbc2/ram.gb
- [ ] mbc2/rom_1Mb.gb
- [ ] mbc2/rom_2Mb.gb
- [ ] mbc2/rom_512kb.gb
- [ ] mbc5/rom_16Mb.gb
- [ ] mbc5/rom_1Mb.gb
- [ ] mbc5/rom_2Mb.gb
- [ ] mbc5/rom_32Mb.gb
- [ ] mbc5/rom_4Mb.gb
- [ ] mbc5/rom_512kb.gb
- [ ] mbc5/rom_64Mb.gb
- [ ] mbc5/rom_8Mb.gb

#### Madness

- [ ] mgb_oam_dma_halt_sprites.gb

#### Manual only

- [ ] sprite_priority.gb

#### Misc

- [ ] bits/unused_hwio-C.gb
- [ ] boot_div-A.gb
- [ ] boot_div-cgb0.gb
- [ ] boot_div-cgbABCDE.gb
- [ ] boot_hwio-C.gb
- [ ] boot_regs-A.gb
- [ ] boot_regs-cgb.gb
- [ ] ppu/vblank_stat_intr-C.gb

