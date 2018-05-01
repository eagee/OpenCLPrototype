//! [1]

 __kernel void createChecksum(__global __read_only unsigned char *fileData, __global __read_only size_t *fileLength, __global int *result)
 {

     // As a test we're literally going to run the same checksum code on all 2000 threads.
     //long index = get_global_id(0);
    int checksum = 0;
    
    if(fileLength > 255)
    {
    
       for(int ix = 0; ix < 255; ix++)
       {
           checksum += fileData[ix];
       }
    }

     // Let us calculate the checksum using the global id (which should be 0-254)
    *result = checksum;
     //*result += fileData[get_global_id(0)];
    //if(checksum == 2553)
    //{
    //    *result = true;
    //}
    //else
    //{
    //    *result = false;
    //}
 }

//! [1]
//! [1]
// __kernel void vectorAdd(__global __read_only int *input1,
//     __global __read_only int *input2,
//     __global __write_only int *output)
// {
//     unsigned int index = get_global_id(0);
//     output[index] = input1[index] + input2[index];
// }
//! [1]
