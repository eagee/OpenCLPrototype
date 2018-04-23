__kernel void createChecksum(__global const unsigned char *fileData, __global size_t length, __global long *result) {

    long checksum = 0;
    for(int ix = 0; ix < length; ix++)
    {
        checksum += fileData[ix];
    }

    *result = checksum;
}
