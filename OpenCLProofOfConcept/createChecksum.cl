__kernel void createChecksum(__global __read_only unsigned char *fileData,
                             __global __read_only long *offset,
                             __global __read_only long *length,
                             __global __write_only long *result) {

    long checksum = 0;

    for(int ix = 0; ix < *length; ix++)
    {
        checksum += fileData[*offset + ix];
    }

    *result = checksum;
}
