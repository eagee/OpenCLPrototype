//! [1]

 __kernel void createChecksum(__global __read_only unsigned char *fileData, __global __read_only size_t *fileLength, __global int *result)
 {
     int checksum = 0;

    // printf("local_id = %ul global_id = %ul\n", get_local_id(0), get_global_id(0));

    // Get the array offset we're going to need to calculate our checksum against fileData
    // (which comprises consecutive chunks of data of *fileLength in size)
    int globalOffset = (get_global_id(0) * fileLength[0]);
    
    // Calculate an integer checksum for the chunk of data specified by the global work item id
    for (long ix = globalOffset; ix < globalOffset + fileLength[0]; ix++)
    {
        checksum += fileData[ix];
    }

     // Assign the reuslts to our checksum output array :)
    result[get_global_id(0)] = checksum;
 }
