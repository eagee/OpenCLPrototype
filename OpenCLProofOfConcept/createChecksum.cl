//! [1]
__kernel void createChecksum(__global __read_only unsigned char *fileData,
                             __global __read_only long *offset,
                             __global __read_only long *length,
                             __global __write_only long *result)
{
    long index = get_global_id(0);
    result[index] = index;
    // long checksum = 0;
    //
    // for(int ix = 0; ix < length[index]; ix++)
    // {
    //     checksum += fileData[offset[index] + ix];
    // }
    //
    // printf("checksum: %d", checksum);
    //
    // result[index] = checksum;
}
//! [1]
