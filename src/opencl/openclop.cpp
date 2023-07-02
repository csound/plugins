#ifdef __MACH__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <vector>
#include <string>
#include <iostream>

const char *cl_error_string(int err);

class OCl {
  int cl_err;
  cl_context context;
  cl_command_queue commands;
  cl_program program;
  std::vector<cl_kernel> kernel;
  std::vector<cl_mem> data;
  void (*err)(std::string s, void *d);
  void *userData;

  static void msg(std::string str, void *userData) {
    if (userData == NULL)
      std::cout << str << std::endl;
  }

public:

  OCl(cl_device_id device_id, int kernels, int arrays, const char *code,
      void (*errs)(std::string s, void *d), void *uData) :
    context(NULL), commands(NULL), program(NULL), kernel(kernels),
    data(arrays), err(errs == NULL ? this->msg : errs),
    userData(uData) {

  context = clCreateContext(0, 1, &device_id, NULL, NULL, &cl_err);
  if (!context) {
    err(cl_error_string(cl_err), userData);
    return;
  }
  commands = clCreateCommandQueue(context, device_id, 0, &cl_err);
  if (!commands) {
    err(cl_error_string(cl_err), userData);
    return;
  }
  program = clCreateProgramWithSource(context, 1, (const char **)&code,
                                      NULL, &cl_err);
  if (!program) {
    err("error creating opencl program\n", userData);
    err(cl_error_string(cl_err), userData);
    return;
  }
  cl_err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (cl_err) {
    char log[2048];
    size_t llen;
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(log),
                          log, &llen);
    err("error building opencl program\n", userData);
    err(cl_error_string(cl_err), userData);
    err(log, userData);
    return;
  }  
  }

  ~OCl() {
  for(cl_mem i : data) clReleaseMemObject(i);
  for(cl_kernel i : kernel) clReleaseKernel(i);
  clReleaseProgram(program);
  clReleaseCommandQueue(commands);
  clReleaseContext(context);
  }

  int execute(int kern, size_t threads) {
    return clEnqueueNDRangeKernel(commands, kernel[kern], 1, NULL, &threads, NULL,
                                  0, NULL, NULL);
  }

  int setKernel(int n, const char *name) {
      kernel[n] = clCreateKernel(program, name, &cl_err);
      return cl_err;
  }

  int alloc(int array, size_t bytes) {
     data[array] = clCreateBuffer(context, 0, bytes, NULL, &cl_err);
     return cl_err;
  }

  int setArg(int kern, int arg, int num) {
   return clSetKernelArg(kernel[kern], num, sizeof(cl_int), &arg);
  }

  int setArg(int kern, float arg, int num) {
   return clSetKernelArg(kernel[kern], num, sizeof(cl_float), &arg);
  }

  int setArg(int kern, double arg, int num) {
   return clSetKernelArg(kernel[kern], num, sizeof(cl_double), &arg);
  }

  int setArg(int kern, cl_mem *arg, int num) {
    return clSetKernelArg(kernel[kern], num, sizeof(cl_mem), arg);
  }

  int setArg(int kern, int *arg, int size, int array, int num) {
    cl_err = clEnqueueWriteBuffer(commands, data[array], 0, 0, size*sizeof(cl_int), arg, 0,
                                  NULL, NULL);
    if (cl_err != CL_SUCCESS)
     return cl_err;
    else
     return clSetKernelArg(kernel[kern], num, sizeof(cl_mem), &data[array]);
   }
 
  int setArg(int kern, float *arg, int size, int array, int num) {
    cl_err = clEnqueueWriteBuffer(commands, data[array], 0, 0, size*sizeof(cl_float), arg, 0,
                                  NULL, NULL);
  if (cl_err != CL_SUCCESS)
    return cl_err;
  else
    return clSetKernelArg(kernel[kern], num, sizeof(cl_mem), &data[array]);
  }

  int setArg(int kern, double *arg, int size, int array, int num) {
    cl_err = clEnqueueWriteBuffer(commands, data[array], 0, 0, size*sizeof(cl_float), arg, 0,
                                  NULL, NULL);
  if (cl_err != CL_SUCCESS)
    return cl_err;
   else
    return clSetKernelArg(kernel[kern], num, sizeof(cl_mem), &data[array]);
  }

  int getData(float *out, int size, int array) {
    return clEnqueueReadBuffer(commands, data[array], CL_TRUE, 0, size*sizeof(cl_float),
                                 out, 0, NULL, NULL);
  }

  int getData(double *out, int size, int array) {
    return clEnqueueReadBuffer(commands, data[array], CL_TRUE, 0, size*sizeof(cl_double),
                                 out, 0, NULL, NULL);
  }

  int getData(int *out, int size, int array) {
    return clEnqueueReadBuffer(commands, data[array], CL_TRUE, 0, size*sizeof(cl_int),
                                 out, 0, NULL, NULL);
  }

  cl_mem *getData(int array) {
    return &data[array];
  }

  int setData(float *out, int size, int array) {
    return clEnqueueWriteBuffer(commands, data[array], 0, 0, size*sizeof(cl_float),
                                 out, 0, NULL, NULL);
  }

  int setData(double *out, int size, int array) {
    return clEnqueueWriteBuffer(commands, data[array], 0, 0, size*sizeof(cl_double),
                                 out, 0, NULL, NULL);
  }

  int setData(int *out, int size, int array) {
    return clEnqueueWriteBuffer(commands, data[array], 0, 0, size*sizeof(cl_int),
                                 out, 0, NULL, NULL);
  }

  int copyData(int array1, int array2, int bytes,
               int offs1 = 0, int offs2 = 0) {
    return clEnqueueCopyBuffer(commands, data[array1], data[array2], offs1, offs2,
                               bytes, 0, NULL, NULL);
  }     

};

const char *cl_error_string(int err) {
  switch (err) {
  case CL_SUCCESS:
    return "Success!";
  case CL_DEVICE_NOT_FOUND:
    return "Device not found.";
  case CL_DEVICE_NOT_AVAILABLE:
    return "Device not available";
  case CL_COMPILER_NOT_AVAILABLE:
    return "Compiler not available";
  case CL_MEM_OBJECT_ALLOCATION_FAILURE:
    return "Memory object allocation failure";
  case CL_OUT_OF_RESOURCES:
    return "Out of resources";
  case CL_OUT_OF_HOST_MEMORY:
    return "Out of host memory";
  case CL_PROFILING_INFO_NOT_AVAILABLE:
    return "Profiling information not available";
  case CL_MEM_COPY_OVERLAP:
    return "Memory copy overlap";
  case CL_IMAGE_FORMAT_MISMATCH:
    return "Image format mismatch";
  case CL_IMAGE_FORMAT_NOT_SUPPORTED:
    return "Image format not supported";
  case CL_BUILD_PROGRAM_FAILURE:
    return "Program build failure";
  case CL_MAP_FAILURE:
    return "Map failure";
  case CL_INVALID_VALUE:
    return "Invalid value";
  case CL_INVALID_DEVICE_TYPE:
    return "Invalid device type";
  case CL_INVALID_PLATFORM:
    return "Invalid platform";
  case CL_INVALID_DEVICE:
    return "Invalid device";
  case CL_INVALID_CONTEXT:
    return "Invalid context";
  case CL_INVALID_QUEUE_PROPERTIES:
    return "Invalid queue properties";
  case CL_INVALID_COMMAND_QUEUE:
    return "Invalid command queue";
  case CL_INVALID_HOST_PTR:
    return "Invalid host pointer";
  case CL_INVALID_MEM_OBJECT:
    return "Invalid memory object";
  case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
    return "Invalid image format descriptor";
  case CL_INVALID_IMAGE_SIZE:
    return "Invalid image size";
  case CL_INVALID_SAMPLER:
    return "Invalid sampler";
  case CL_INVALID_BINARY:
    return "Invalid binary";
  case CL_INVALID_BUILD_OPTIONS:
    return "Invalid build options";
  case CL_INVALID_PROGRAM:
    return "Invalid program";
  case CL_INVALID_PROGRAM_EXECUTABLE:
    return "Invalid program executable";
  case CL_INVALID_KERNEL_NAME:
    return "Invalid kernel name";
  case CL_INVALID_KERNEL_DEFINITION:
    return "Invalid kernel definition";
  case CL_INVALID_KERNEL:
    return "Invalid kernel";
  case CL_INVALID_ARG_INDEX:
    return "Invalid argument index";
  case CL_INVALID_ARG_VALUE:
    return "Invalid argument value";
  case CL_INVALID_ARG_SIZE:
    return "Invalid argument size";
  case CL_INVALID_KERNEL_ARGS:
    return "Invalid kernel arguments";
  case CL_INVALID_WORK_DIMENSION:
    return "Invalid work dimension";
  case CL_INVALID_WORK_GROUP_SIZE:
    return "Invalid work group size";
  case CL_INVALID_WORK_ITEM_SIZE:
    return "Invalid work item size";
  case CL_INVALID_GLOBAL_OFFSET:
    return "Invalid global offset";
  case CL_INVALID_EVENT_WAIT_LIST:
    return "Invalid event wait list";
  case CL_INVALID_EVENT:
    return "Invalid event";
  case CL_INVALID_OPERATION:
    return "Invalid operation";
  case CL_INVALID_GL_OBJECT:
    return "Invalid OpenGL object";
  case CL_INVALID_BUFFER_SIZE:
    return "Invalid buffer size";
  case CL_INVALID_MIP_LEVEL:
    return "Invalid mip-map level";
  default:
    return "Unknown error";
  }
}
