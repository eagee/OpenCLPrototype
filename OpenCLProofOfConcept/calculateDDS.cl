// OpenCL program to generate all DDS checksum values for file data and work count specified.
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_nv_pragma_unroll : enable

// Types we'll use for calculating the md5
#define BLOCK_SIZE 2048
#define TOTAL_CHECKSUMS ((1024*1024*2) - 8) //1048568 

typedef struct 
{
    ulong values[TOTAL_CHECKSUMS];
} DdsChecksums;

ulong createDdsChecksum64(__global ulong *buffer)
{
    ulong sum = 0;

    #pragma unroll
    int len = BLOCK_SIZE / sizeof(ulong);
    for(int i = 0; i < len; i++)
    {
        sum += buffer[i];
    }
    
    return sum;
}

/// This method will run coalesced, generating a checksum for each BLOCK_SIZE of bytes triggered by the global_id of the active work item.
/// fileData points to a buffer of file information (at leat 1MB long)
/// fileLength specifies the lenght of the fileData (again, it needs to be at least 1MB long)
/// Results buffer points to an array of cl_ulong values that needs to be at least 1048568 (TOTAL_CHECKSUMS) in length
/// fileLength specifies the total number of bytes to be processed, not the total number of ulong values.
__kernel void calculateDDS(__global ulong *fileData, __global size_t *fileLength, __global DdsChecksums *resultsBuffer)
{
    // Global id should be between 0 and TOTAL_CHECKSUMS (since that represents our work item count and the number of checksums
    // we wish to generate).
    uint offset = get_global_id(0);

    if(offset * sizeof(ulong) + BLOCK_SIZE >= *fileLength)
    {
        return;
    }

   // Generate our dds checksum and store the results based on the current global id (which should be the current work item)
   resultsBuffer->values[offset] = createDdsChecksum64(&fileData[offset]);
}
