/*
 * U-Boot Image Header
 * -------------------
 *
 * This header provides the necessary header structure
 * to decode a legacy U-Boot image made with mkimage.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 */

#ifndef UBOOT_IMAGE_H_
#define UBOOT_IMAGE_H_

#include "Util/ct_assert.h"
#include "Util/bit_helpers.h"

#include <stdint.h>
#include <string.h>

inline size_t strnlen(const char* str, size_t maxLen) {
    size_t len = 0;
    do {
        if(str[len] == '\0') return len;
    } while(len++ < maxLen);
    return maxLen;
}

/*
 * -----------------------------------------------------------------------------------------------
 * -----------------------------------------------------------------------------------------------
 * -----------------------------------------------------------------------------------------------
 * Begin Copyright Section
 *
 * These are extracts from the U-Boot project header with license
 * terms specified below. Including the header unmodified is
 * considered normal use. However this project can't satisfy all
 * of the #include statements in the original header, and as such
 * it has been truncated to just the required sections.
 * -----------------------------------------------------------------------------------------------
 */

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 ********************************************************************
 * NOTE: This header file defines an interface to U-Boot. Including
 * this (unmodified) header file in another file is considered normal
 * use of U-Boot, and does *not* fall under the heading of "derived
 * work".
 ********************************************************************
 */

/* An invalid size, meaning that the image size is not known */
#define IMAGE_SIZE_INVAL    (-1UL)

enum ih_category {
    IH_ARCH,
    IH_COMP,
    IH_OS,
    IH_TYPE,
    IH_PHASE,

    IH_COUNT,
};

/*
 * Operating System Codes
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum {
    IH_OS_INVALID   = 0,    /* Invalid OS   */
    IH_OS_OPENBSD,          /* OpenBSD  */
    IH_OS_NETBSD,           /* NetBSD   */
    IH_OS_FREEBSD,          /* FreeBSD  */
    IH_OS_4_4BSD,           /* 4.4BSD   */
    IH_OS_LINUX,            /* Linux    */
    IH_OS_SVR4,             /* SVR4     */
    IH_OS_ESIX,             /* Esix     */
    IH_OS_SOLARIS,          /* Solaris  */
    IH_OS_IRIX,             /* Irix     */
    IH_OS_SCO,              /* SCO      */
    IH_OS_DELL,             /* Dell     */
    IH_OS_NCR,              /* NCR      */
    IH_OS_LYNXOS,           /* LynxOS   */
    IH_OS_VXWORKS,          /* VxWorks  */
    IH_OS_PSOS,             /* pSOS     */
    IH_OS_QNX,              /* QNX      */
    IH_OS_U_BOOT,           /* Firmware */
    IH_OS_RTEMS,            /* RTEMS    */
    IH_OS_ARTOS,            /* ARTOS    */
    IH_OS_UNITY,            /* Unity OS */
    IH_OS_INTEGRITY,        /* INTEGRITY    */
    IH_OS_OSE,              /* OSE      */
    IH_OS_PLAN9,            /* Plan 9   */
    IH_OS_OPENRTOS,         /* OpenRTOS */
    IH_OS_ARM_TRUSTED_FIRMWARE,     /* ARM Trusted Firmware */
    IH_OS_TEE,              /* Trusted Execution Environment */
    IH_OS_OPENSBI,          /* RISC-V OpenSBI */
    IH_OS_EFI,              /* EFI Firmware (e.g. GRUB2) */

    IH_OS_COUNT,
};

/*
 * CPU Architecture Codes (supported by Linux)
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum {
    IH_ARCH_INVALID = 0,    /* Invalid CPU  */
    IH_ARCH_ALPHA,          /* Alpha    */
    IH_ARCH_ARM,            /* ARM      */
    IH_ARCH_I386,           /* Intel x86    */
    IH_ARCH_IA64,           /* IA64     */
    IH_ARCH_MIPS,           /* MIPS     */
    IH_ARCH_MIPS64,         /* MIPS  64 Bit */
    IH_ARCH_PPC,            /* PowerPC  */
    IH_ARCH_S390,           /* IBM S390 */
    IH_ARCH_SH,             /* SuperH   */
    IH_ARCH_SPARC,          /* Sparc    */
    IH_ARCH_SPARC64,        /* Sparc 64 Bit */
    IH_ARCH_M68K,           /* M68K     */
    IH_ARCH_NIOS,           /* Nios-32  */
    IH_ARCH_MICROBLAZE,     /* MicroBlaze   */
    IH_ARCH_NIOS2,          /* Nios-II  */
    IH_ARCH_BLACKFIN,       /* Blackfin */
    IH_ARCH_AVR32,          /* AVR32    */
    IH_ARCH_ST200,          /* STMicroelectronics ST200  */
    IH_ARCH_SANDBOX,        /* Sandbox architecture (test only) */
    IH_ARCH_NDS32,          /* ANDES Technology - NDS32  */
    IH_ARCH_OPENRISC,       /* OpenRISC 1000  */
    IH_ARCH_ARM64,          /* ARM64    */
    IH_ARCH_ARC,            /* Synopsys DesignWare ARC */
    IH_ARCH_X86_64,         /* AMD x86_64, Intel and Via */
    IH_ARCH_XTENSA,         /* Xtensa   */
    IH_ARCH_RISCV,          /* RISC-V */

    IH_ARCH_COUNT,
};

/*
 * Image Types
 *
 * "Standalone Programs" are directly runnable in the environment
 *  provided by U-Boot; it is expected that (if they behave
 *  well) you can continue to work in U-Boot after return from
 *  the Standalone Program.
 * "OS Kernel Images" are usually images of some Embedded OS which
 *  will take over control completely. Usually these programs
 *  will install their own set of exception handlers, device
 *  drivers, set up the MMU, etc. - this means, that you cannot
 *  expect to re-enter U-Boot except by resetting the CPU.
 * "RAMDisk Images" are more or less just data blocks, and their
 *  parameters (address, size) are passed to an OS kernel that is
 *  being started.
 * "Multi-File Images" contain several images, typically an OS
 *  (Linux) kernel image and one or more data images like
 *  RAMDisks. This construct is useful for instance when you want
 *  to boot over the network using BOOTP etc., where the boot
 *  server provides just a single image file, but you want to get
 *  for instance an OS kernel and a RAMDisk image.
 *
 *  "Multi-File Images" start with a list of image sizes, each
 *  image size (in bytes) specified by an "uint32_t" in network
 *  byte order. This list is terminated by an "(uint32_t)0".
 *  Immediately after the terminating 0 follow the images, one by
 *  one, all aligned on "uint32_t" boundaries (size rounded up to
 *  a multiple of 4 bytes - except for the last file).
 *
 * "Firmware Images" are binary images containing firmware (like
 *  U-Boot or FPGA images) which usually will be programmed to
 *  flash memory.
 *
 * "Script files" are command sequences that will be executed by
 *  U-Boot's command interpreter; this feature is especially
 *  useful when you configure U-Boot to use a real shell (hush)
 *  as command interpreter (=> Shell Scripts).
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum image_type_t {
    IH_TYPE_INVALID     = 0,/* Invalid Image        */
    IH_TYPE_STANDALONE,     /* Standalone Program       */
    IH_TYPE_KERNEL,         /* OS Kernel Image      */
    IH_TYPE_RAMDISK,        /* RAMDisk Image        */
    IH_TYPE_MULTI,          /* Multi-File Image     */
    IH_TYPE_FIRMWARE,       /* Firmware Image       */
    IH_TYPE_SCRIPT,         /* Script file          */
    IH_TYPE_FILESYSTEM,     /* Filesystem Image (any type)  */
    IH_TYPE_FLATDT,         /* Binary Flat Device Tree Blob */
    IH_TYPE_KWBIMAGE,       /* Kirkwood Boot Image      */
    IH_TYPE_IMXIMAGE,       /* Freescale IMXBoot Image  */
    IH_TYPE_UBLIMAGE,       /* Davinci UBL Image        */
    IH_TYPE_OMAPIMAGE,      /* TI OMAP Config Header Image  */
    IH_TYPE_AISIMAGE,       /* TI Davinci AIS Image     */
    /* OS Kernel Image, can run from any load address */
    IH_TYPE_KERNEL_NOLOAD,
    IH_TYPE_PBLIMAGE,       /* Freescale PBL Boot Image */
    IH_TYPE_MXSIMAGE,       /* Freescale MXSBoot Image  */
    IH_TYPE_GPIMAGE,        /* TI Keystone GPHeader Image   */
    IH_TYPE_ATMELIMAGE,     /* ATMEL ROM bootable Image */
    IH_TYPE_SOCFPGAIMAGE,   /* Altera SOCFPGA CV/AV Preloader */
    IH_TYPE_X86_SETUP,      /* x86 setup.bin Image      */
    IH_TYPE_LPC32XXIMAGE,   /* x86 setup.bin Image      */
    IH_TYPE_LOADABLE,       /* A list of typeless images    */
    IH_TYPE_RKIMAGE,        /* Rockchip Boot Image      */
    IH_TYPE_RKSD,           /* Rockchip SD card     */
    IH_TYPE_RKSPI,          /* Rockchip SPI image       */
    IH_TYPE_ZYNQIMAGE,      /* Xilinx Zynq Boot Image */
    IH_TYPE_ZYNQMPIMAGE,    /* Xilinx ZynqMP Boot Image */
    IH_TYPE_ZYNQMPBIF,      /* Xilinx ZynqMP Boot Image (bif) */
    IH_TYPE_FPGA,           /* FPGA Image */
    IH_TYPE_VYBRIDIMAGE,    /* VYBRID .vyb Image */
    IH_TYPE_TEE,            /* Trusted Execution Environment OS Image */
    IH_TYPE_FIRMWARE_IVT,   /* Firmware Image with HABv4 IVT */
    IH_TYPE_PMMC,           /* TI Power Management Micro-Controller Firmware */
    IH_TYPE_STM32IMAGE,     /* STMicroelectronics STM32 Image */
    IH_TYPE_SOCFPGAIMAGE_V1,/* Altera SOCFPGA A10 Preloader */
    IH_TYPE_MTKIMAGE,       /* MediaTek BootROM loadable Image */
    IH_TYPE_IMX8MIMAGE,     /* Freescale IMX8MBoot Image    */
    IH_TYPE_IMX8IMAGE,      /* Freescale IMX8Boot Image */
    IH_TYPE_COPRO,          /* Coprocessor Image for remoteproc*/
    IH_TYPE_SUNXI_EGON,     /* Allwinner eGON Boot Image */
    IH_TYPE_SUNXI_TOC0,     /* Allwinner TOC0 Boot Image */
    IH_TYPE_FDT_LEGACY,     /* Binary Flat Device Tree Blob in a Legacy Image */
    IH_TYPE_RENESAS_SPKG,   /* Renesas SPKG image */

    IH_TYPE_COUNT,          /* Number of image types */
};

/*
 * Compression Types
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum {
    IH_COMP_NONE    = 0,    /*  No   Compression Used   */
    IH_COMP_GZIP,           /* gzip  Compression Used   */
    IH_COMP_BZIP2,          /* bzip2 Compression Used   */
    IH_COMP_LZMA,           /* lzma  Compression Used   */
    IH_COMP_LZO,            /* lzo   Compression Used   */
    IH_COMP_LZ4,            /* lz4   Compression Used   */
    IH_COMP_ZSTD,           /* zstd  Compression Used   */

    IH_COMP_COUNT,
};

#define IH_MAGIC    0x27051956  /* Image Magic Number       */
#define IH_NMLEN    32          /* Image Name Length        */

/*
 * Legacy format image header,
 * all data in network byte order (aka natural aka bigendian).
 */
struct legacy_img_hdr {
    uint32_t ih_magic;          /* Image Header Magic Number */
    uint32_t ih_hcrc;           /* Image Header CRC Checksum */
    uint32_t ih_time;           /* Image Creation Timestamp  */
    uint32_t ih_size;           /* Image Data Size           */
    uint32_t ih_load;           /* Data  Load  Address       */
    uint32_t ih_ep;             /* Entry Point Address       */
    uint32_t ih_dcrc;           /* Image Data CRC Checksum   */
    uint8_t  ih_os;             /* Operating System          */
    uint8_t  ih_arch;           /* CPU architecture          */
    uint8_t  ih_type;           /* Image Type                */
    uint8_t  ih_comp;           /* Compression Type          */
    uint8_t  ih_name[IH_NMLEN]; /* Image Name                */
};

/*
 * Some systems (for example LWMON) have very short watchdog periods;
 * we must make sure to split long operations like memmove() or
 * checksum calculations into reasonable chunks.
 */
#ifndef CHUNKSZ
#define CHUNKSZ (64 * 1024)
#endif

#ifndef CHUNKSZ_CRC32
#define CHUNKSZ_CRC32 (64 * 1024)
#endif


#define uimage_to_cpu(x)        be32_to_cpu(x)
#define cpu_to_uimage(x)        cpu_to_be32(x)

/*******************************************************************/
/* Legacy format specific code (prefixed with image_) */
/*******************************************************************/
static inline uint32_t image_get_header_size(void)
{
    return sizeof(struct legacy_img_hdr);
}

#define image_get_hdr_l(f) \
    static inline uint32_t image_get_##f(const struct legacy_img_hdr *hdr) \
    { \
        return uimage_to_cpu(hdr->ih_##f); \
    }
image_get_hdr_l(magic)      /* image_get_magic */
image_get_hdr_l(hcrc)       /* image_get_hcrc */
image_get_hdr_l(time)       /* image_get_time */
image_get_hdr_l(size)       /* image_get_size */
image_get_hdr_l(load)       /* image_get_load */
image_get_hdr_l(ep)         /* image_get_ep */
image_get_hdr_l(dcrc)       /* image_get_dcrc */

#define image_get_hdr_b(f) \
    static inline uint8_t image_get_##f(const struct legacy_img_hdr *hdr) \
    { \
        return hdr->ih_##f; \
    }
image_get_hdr_b(os)         /* image_get_os */
image_get_hdr_b(arch)       /* image_get_arch */
image_get_hdr_b(type)       /* image_get_type */
image_get_hdr_b(comp)       /* image_get_comp */

static inline char *image_get_name(const struct legacy_img_hdr *hdr)
{
    return (char *)hdr->ih_name;
}

static inline uint32_t image_get_data_size(const struct legacy_img_hdr *hdr)
{
    return image_get_size(hdr);
}

/**
 * image_get_data - get image payload start address
 * @hdr: image header
 *
 * image_get_data() returns address of the image payload. For single
 * component images it is image data start. For multi component
 * images it points to the null terminated table of sub-images sizes.
 *
 * returns:
 *     image payload data start address
 */
static inline ulong image_get_data(const struct legacy_img_hdr *hdr)
{
    return ((ulong)hdr + image_get_header_size());
}

static inline uint32_t image_get_image_size(const struct legacy_img_hdr *hdr)
{
    return (image_get_size(hdr) + image_get_header_size());
}

static inline ulong image_get_image_end(const struct legacy_img_hdr *hdr)
{
    return ((ulong)hdr + image_get_image_size(hdr));
}

#define image_set_hdr_l(f) \
    static inline void image_set_##f(struct legacy_img_hdr *hdr, uint32_t val) \
    { \
        hdr->ih_##f = cpu_to_uimage(val); \
    }
image_set_hdr_l(magic)      /* image_set_magic */
image_set_hdr_l(hcrc)       /* image_set_hcrc */
image_set_hdr_l(time)       /* image_set_time */
image_set_hdr_l(size)       /* image_set_size */
image_set_hdr_l(load)       /* image_set_load */
image_set_hdr_l(ep)     /* image_set_ep */
image_set_hdr_l(dcrc)       /* image_set_dcrc */

#define image_set_hdr_b(f) \
    static inline void image_set_##f(struct legacy_img_hdr *hdr, uint8_t val) \
    { \
        hdr->ih_##f = val; \
    }
image_set_hdr_b(os)     /* image_set_os */
image_set_hdr_b(arch)       /* image_set_arch */
image_set_hdr_b(type)       /* image_set_type */
image_set_hdr_b(comp)       /* image_set_comp */

static inline void image_set_name(struct legacy_img_hdr *hdr, const char *name)
{
    /*
     * This is equivalent to: strncpy(image_get_name(hdr), name, IH_NMLEN);
     *
     * Use the tortured code below to avoid a warning with gcc 12. We do not
     * want to include a nul terminator if the name is of length IH_NMLEN
     */
    memcpy(image_get_name(hdr), name, strnlen(name, IH_NMLEN));
}

int image_check_hcrc(const struct legacy_img_hdr *hdr);
int image_check_dcrc(const struct legacy_img_hdr *hdr);

static inline int image_check_magic(const struct legacy_img_hdr *hdr)
{
    return (image_get_magic(hdr) == IH_MAGIC);
}

static inline int image_check_type(const struct legacy_img_hdr *hdr, uint8_t type)
{
    return (image_get_type(hdr) == type);
}

static inline int image_check_arch(const struct legacy_img_hdr *hdr, uint8_t arch)
{
    return (image_get_arch(hdr) == arch) ||
        (image_get_arch(hdr) == IH_ARCH_ARM && arch == IH_ARCH_ARM64);
}

static inline int image_check_os(const struct legacy_img_hdr *hdr, uint8_t os)
{
    return (image_get_os(hdr) == os);
}

/*
 * -----------------------------------------------------------------------------------------------
 * End Copyright section
 * -----------------------------------------------------------------------------------------------
 * -----------------------------------------------------------------------------------------------
 * -----------------------------------------------------------------------------------------------
 */

#endif /* UBOOT_IMAGE_H_ */
