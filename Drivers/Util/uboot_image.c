/*
 * U-Boot Image Header
 * -------------------
 *
 * This header provides the necessary header structure
 * to decode a legacy U-Boot image made with mkimage.
 *
 */

#include "uboot_image.h"
#include "crc32.h"
#include <string.h>

/*****************************************************************************/
/* Legacy format routines */
/*****************************************************************************/
int image_check_hcrc(const struct legacy_img_hdr *hdr)
{
    ulong hcrc;
    ulong len = image_get_header_size();
    struct legacy_img_hdr header;

    /* Copy header so we can blank CRC field for re-calculation */
    memmove(&header, (char *)hdr, image_get_header_size());
    image_set_hcrc(&header, 0);

    hcrc = crc32(0, (unsigned char *)&header, len);

    return (hcrc == image_get_hcrc(hdr));
}

int image_check_dcrc(const struct legacy_img_hdr *hdr)
{
    ulong data = image_get_data(hdr);
    ulong len = image_get_data_size(hdr);
    ulong dcrc = crc32_wd(0, (unsigned char *)data, len, CHUNKSZ_CRC32);
    return (dcrc == image_get_dcrc(hdr));
}
