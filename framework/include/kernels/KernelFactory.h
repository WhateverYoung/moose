#ifndef KERNELFACTORY_H
#define KERNELFACTORY_H

#include "Kernel.h"

// System includes
#include <map>
#include <string>
#include <vector>
#include <typeinfo>

// LibMesh includes
#include <parameters.h>

// forward declarations
class MooseSystem;

/**
 * Typedef to make things easier.
 */
typedef Kernel * (*kernelBuildPtr)(std::string name, MooseSystem & moose_system, InputParameters parameters);

/**
 * Typedef to make things easier.
 */
typedef InputParameters (*kernelParamsPtr)();

/**
 * Typedef to hide implementation details
 */
typedef std::vector<Kernel *>::iterator KernelIterator;

/**
 * Typedef to hide implementation details
 */
typedef std::vector<std::string>::iterator KernelNamesIterator;

/**
 * Templated build function used for generating function pointers to build classes on demand.
 */
template<typename KernelType>
Kernel * buildKernel(std::string name, MooseSystem & moose_system, InputParameters parameters)
{
  return new KernelType(name, moose_system, parameters);
}

/**
 * Responsible for building Kernels on demand and storing them for retrieval
 */
class KernelFactory
{
public:
  static KernelFactory * instance();

  template<typename KernelType> 
  void registerKernel(std::string name)
  {
    name_to_build_pointer[name]=&buildKernel<KernelType>;
    name_to_params_pointer[name]=&validParams<KernelType>;
  }

  Kernel * add(std::string kernel_name,
               std::string name,
               MooseSystem & moose_system,
               InputParameters parameters);
  

  Kernel * add(std::string kernel_name,
               std::string name,
               MooseSystem & moose_system,
               InputParameters parameters,
               unsigned int block_id);
  
  InputParameters getValidParams(std::string name);
  
  KernelIterator activeKernelsBegin(THREAD_ID tid);
  KernelIterator activeKernelsEnd(THREAD_ID tid);

  KernelIterator blockKernelsBegin(THREAD_ID tid, unsigned int block_id);
  KernelIterator blockKernelsEnd(THREAD_ID tid, unsigned int block_id);

  bool activeKernelBlocks(std::set<subdomain_id_type> & set_buffer) const;

  KernelNamesIterator registeredKernelsBegin();
  KernelNamesIterator registeredKernelsEnd();

  void updateActiveKernels(THREAD_ID tid);
  
private:
  KernelFactory();

  virtual ~KernelFactory();
  
  std::map<std::string, kernelBuildPtr> name_to_build_pointer;
  std::map<std::string, kernelParamsPtr> name_to_params_pointer;

  std::vector<std::string> _registered_kernel_names;
  std::vector<std::vector<Kernel *> > active_kernels;
  std::vector<std::vector<Kernel *> > all_kernels;

  std::vector<std::map<unsigned int, std::vector<Kernel *> > > block_kernels;
  std::vector<std::map<unsigned int, std::vector<Kernel *> > > all_block_kernels;

};

#endif //KERNELFACTORY_H
