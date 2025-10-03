#ifndef BOREALOS_SETTINGS_H
#define BOREALOS_SETTINGS_H

// These are basic settings for the Borealos operating system.
// These are compile time settings, for stuff like enabling/disabling features.

// Enable or disable features here.
// 1 = enabled, 0 = disabled

// Only the features that exist here can be enabled/disabled.

/// Enable ATA/ATAPI support
/// Disabling this means no ATA/ATAPI drivers will be compiled into the kernel.
/// This also disables support for IDE disks in the disk subsystem.
/// If you don't need ATA/ATAPI support (e.g. you're only using SCSI/SATA drives).
/// Unfortunately due to some limitations of how ATA/ATAPI works, it is very slow to boot with ATA/ATAPI support enabled IF no drives are present (it has to timeout waiting for drives that aren't there).
/// You can disable this to improve boot time (massively on an R7 3700x 750ms to 0ms) and reduce kernel size (slightly).
#define BOREALOS_ENABLE_ATA 1

#endif //BOREALOS_SETTINGS_H