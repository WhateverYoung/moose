#ifndef STABILIZER_H
#define STABILIZER_H

#include "Moose.h"
#include "Kernel.h"

// LibMesh includes
#include <parameters.h>
#include <point.h>
#include <vector_value.h>

// System includes
#include <string>

//forward declarations
class Stabilizer;

template<>
InputParameters validParams<Stabilizer>();

/**
 * Stabilizers compute modified test function spaces to stabilize oscillating solutions.
 */
class Stabilizer : protected Kernel
{
public:

  /**
   * Constructor
   *
   * @param name The name given to the initial condition in the input file.
   * @param moose_system The reference to the MooseSystem that this object is contained within
   * @param parameters The parameters object holding data for the class to use.
   */
  Stabilizer(std::string name, MooseSystem & moose_system, InputParameters parameters);

  /**
   * This pure virtual must be overriden by derived classes!
   *
   * This is where the stabilization scheme should compute the test function space.
   * This usually entails multipliying _phi by something and storing it in _test
   */
  virtual void computeTestFunctions() = 0;

  // Functions from Kernel that we _do_ want to provide
  Kernel::variable;
  Kernel::subdomainSetup;

private:

  // Just here to satisfy the pure virtual
  virtual Real computeQpResidual(){ return 0; }
};

#endif //STABILIZER_H
