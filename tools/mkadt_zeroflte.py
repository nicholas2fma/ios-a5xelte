#!/usr/bin/env python3
"""Build the zeroflte (SM-G920F, Exynos 7420) ADT, starting from the herolte one.

Values come from the phone: probe photo (MPIDR 0x100, A53 r0p2), its BOOT DTB
(memory, GIC, PSCI, reserved-memory, ufs@0x15570000) and the Linux log (24 MHz).
None is verified under XNU yet.
"""
import argparse
import os
from pathlib import Path

import adt
import mkadt
import mkadt_herolte as hero
from mkadt_herolte import child, set_prop

DRAM_BASE = 0x50000000      # above S-Boot's images and Samsung's logs
DRAM_END = 0xe2000000       # below the first fixed reservation, 0xe2600000
TIMEBASE = 24000000
A53_HZ, A57_HZ = 1500000000, 2100000000


def build(mb):
    root = hero.build(dram=(DRAM_BASE, mb << 20))
    set_prop(root, 'compatible', mkadt.cstr('D22AP') + mkadt.cstr('iPhone10,3') +
             mkadt.cstr('samsung,a5xelte') + mkadt.cstr('samsung,zeroflte') +
             mkadt.cstr('samsung,exynos7420') + mkadt.cstr('AppleARM'))
    chosen = child(root, 'chosen')
    set_prop(chosen, 'firmware-version', mkadt.cstr('zeroflte-entry'))
    set_prop(chosen, 'system-firmware-version', mkadt.cstr('zeroflte-entry'))
    cpus = child(root, 'cpus')
    for logical in range(8):
        cpu = child(cpus, f'cpu{logical}')
        set_prop(cpu, 'timebase-frequency', mkadt.u32(TIMEBASE))
        set_prop(cpu, 'peripheral-frequency', mkadt.u32(TIMEBASE))
        set_prop(cpu, 'fixed-frequency', mkadt.u32(TIMEBASE))
        set_prop(cpu, 'clock-frequency', mkadt.u32(A57_HZ if logical >= 4 else A53_HZ))
        if logical >= 4:
            # SweetCPU matches 'arm,cortex-a53' (KernelConfigTables).
            set_prop(cpu, 'compatible', mkadt.cstr('arm,cortex-a57') +
                     mkadt.cstr('arm,cortex-a53') + mkadt.cstr('ARM,v8'))
    arm_io = child(root, 'arm-io')
    set_prop(arm_io, 'compatible', mkadt.cstr('samsung,exynos7420-io') + mkadt.cstr('arm-io'))
    disp = child(arm_io, 'disp0')
    set_prop(disp, 'compatible', mkadt.cstr('samsung,a5xelte-display') +
             mkadt.cstr('samsung,zeroflte-display'))
    set_prop(child(root, 'herolte-ufs'), 'reg', mkadt.u64(0x15570000, 0x3000))
    return root


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--mb', type=int, default=512, help='MiB of RAM for XNU, from 0x50000000')
    args = parser.parse_args()
    if not 0 < args.mb << 20 <= DRAM_END - DRAM_BASE:
        raise SystemExit('RAM outside the free window')
    blob = adt.serialize(build(args.mb))
    args.output.write_bytes(blob)
    print(f'{args.output}: {len(blob)} bytes')
