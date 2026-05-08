This is an Open Source (MIT License) bare-metal OS project with direct hardware access.

It features a simple, DOS-like terminal and utilizes the GRUB Bootloader for the kernel loading process.

FEATURES:

- VGA Text Mode: Boots directly into a functional text interface.

- x64 Long Mode: Running on ELF64 / BIOS architecture.

- STG Command (Stub): The GUI starting feature is currently an empty stub (placeholder). The required blitting engine is not yet implemented, making this a prime area for development.

- Core Commands: Supports HELP, CLEAR, STG, and VER.

- Terminal Logic: Includes basic text writing and line-editing features with boundary checking.

The system is stable and ready for further architectural development.

  ~Mila
