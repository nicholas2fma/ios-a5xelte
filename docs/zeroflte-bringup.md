# zeroflte (Galaxy S6 SM-G920F, Exynos 7420)

Based on the `herolte` port. The kernel is built with `MACHINE_CONFIG=HEROLTE`
plus `-DARM64_BOARD_CONFIG_ZEROFLTE` (patch 0300), so the artifact is still named
`kernel.development.herolte`.

Evidence on the phone (stage-0 probe, photo): EL1, MMU off, Image 0x40205000,
initrd 0x43000000, FDT 0x4a000000, MIDR 0x410fd032 (Cortex-A53 r0p2), MPIDR
0x100, MMFR0 0x1122 (no 16 KiB pages), ISAR0 0x11120 (no LSE), CNTFRQ 0, screen
text and PMU reset to recovery both work.

Patch 0300 turns off everything that programs Exynos 8890 hardware: keys, touch,
fuel gauge, HeroGPU, clocks and PMIC writes (the S6 has an S2MPS15, not an
S2MPS16). Never pass the `heroaudio` boot-arg. It also applies the Cortex-A53 r0p2 clean-cache errata workaround.

XNU trials write only BOOT. Never write EFS, RECOVERY or bootloader partitions.
