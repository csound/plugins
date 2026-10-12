// -*- c++ -*-
/* conv.cu

  (c) Victor Lazzarini, 2013

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <csdl.h>

#define S(x) sizeof(x)

__global__ void convol(cs_float *out, cs_float *del, cs_float *coefs, int irsize, int rp, int vsize) {
  int t = (threadIdx.x + blockIdx.x*blockDim.x);
  if(t >= irsize*vsize) return;
  int n =  t%vsize;  /* sample index */
  int h =  t/vsize;  /* coeff index */
  int end = irsize+vsize;
  rp += n + h; /* read point, oldest -> newest */
  out[t] = del[rp < end ? rp : rp%end]*coefs[irsize-1-h];  /* single tap */
  if(t >= vsize) return;
  __syncthreads();
  cs_float a = 0.0;
  for(int i=1, j=vsize; i < irsize; i++, j+=vsize)
    a +=  out[n + j]; /* mix all taps */   
  out[n] += a;    
}

// __device__ double atomicAdd(double* address, double val)
// {
//     unsigned long long int* address_as_ull =
//                               (unsigned long long int*)address;
//     unsigned long long int old = *address_as_ull, assumed;
//     do {
//         assumed = old;
//         old = atomicCAS(address_as_ull, assumed,
//                         __double_as_longlong(val +
//                                __longlong_as_double(assumed)));
//     } while (assumed != old);
//     return __longlong_as_double(old);
// }

// __global__ void convol2(cs_float *out, cs_float *del, cs_float *coefs, int irsize, int rp, int vsize) {
//   int t = (threadIdx.x + blockIdx.x*blockDim.x);
//   if(t >= irsize*vsize) return;
//   int n =  t%vsize;  /* sample index */
//   int h =  t/vsize;  /* coeff index */
//   int end = irsize+vsize;
//   rp += n + h; /* read point, oldest -> newest */
//   cs_float s = del[rp < end ? rp : rp%end]*coefs[irsize-1-h];  /* single tap */
//   t == n ? out[n] = s : atomicAdd(&out[n], s);
// }

typedef struct _CONV {
  OPDS h;
  cs_float *aout, *asig, *ifn;
  cs_float *coeffs, *out, *del;
  int wp, irsize;
  int blocks, threads;
} CONV;


static int destroy_conv(CSOUND *csound, void *pp){
  CONV *p = (CONV *) pp;
  cudaFree(p->coeffs);
  cudaFree(p->del);
  cudaFree(p->out);
  return OK;
}

static int conv_init(CSOUND *csound, CONV *p){

  FUNC *ftab = csound->FTFind(csound, p->ifn);
  if (ftab == NULL)
    return csound->InitError(csound, "could not find table %d\n",
                             (int) *p->ifn);
  int irsize = ftab->flen;
  int nsmps = CS_KSMPS;
  int threads = irsize*nsmps;
 
  cudaMalloc(&p->coeffs, sizeof(cs_float)*irsize);   
  cudaMemcpy(p->coeffs, ftab->ftable, sizeof(cs_float)*irsize, 
            cudaMemcpyHostToDevice); 
   
  cudaMalloc(&p->del, sizeof(cs_float)*(irsize+nsmps)); 
  cudaMalloc(&p->out, sizeof(cs_float)*threads); 
  cudaMemset(p->del,0,sizeof(cs_float)*(irsize+nsmps));
  cudaMemset(p->out, 0, sizeof(cs_float)*threads);
  
  p->wp = 0;
  p->irsize = irsize;

  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);
  int blockspt = deviceProp.maxThreadsPerBlock;
  csound->Message(csound, "CUDAconv: using device %s (capability %d.%d)\n", 
        deviceProp.name,deviceProp.major, deviceProp.minor);

  p->blocks = (threads + blockspt - 1)/blockspt;
  p->threads = threads > blockspt ? blockspt : threads;

  if(csound->GetDebug(csound))
   csound->Message(csound, "blocks %d, threads %d - %d\n", p->blocks, p->threads, threads);

  return OK;

}
/* the delay size is irsize + vsize so that
   we can shift in a whole block of samples */
int conv_perf(CSOUND *csound, CONV *p){

   int nsmps = CS_KSMPS;
   cs_float *sig = p->asig, *aout = p->aout;
   cs_float *del = p->del, *out = p->out, *coefs = p->coeffs;
   int irsize = p->irsize;
   int wp = p->wp;

  if(wp > irsize) {
     int front = wp - irsize; 
     cudaMemcpy(&del[wp], sig, sizeof(cs_float)*(nsmps-front), cudaMemcpyHostToDevice);
     cudaMemcpy(del, &sig[nsmps-front], sizeof(cs_float)*front, cudaMemcpyHostToDevice);
  } 
  else cudaMemcpy(&del[wp], sig, sizeof(cs_float)*nsmps, cudaMemcpyHostToDevice); 
  
  wp = (wp+nsmps)%(irsize+nsmps); /* wp is now the oldest sample in the delay */
  convol<<<p->blocks,p->threads>>>(out, del, coefs, irsize, wp, nsmps);

  cudaMemcpy(aout, out, sizeof(cs_float)*nsmps, cudaMemcpyDeviceToHost); 
  p->wp = wp;
  return OK;
}

static OENTRY localops[] = {
  {"cudaconv", S(CONV), 0, "a", "ai", (SUBR) conv_init,
   (SUBR) conv_perf, (SUBR) destroy_conv, NULL, 0},
};

extern "C" {
  LINKAGE
}
