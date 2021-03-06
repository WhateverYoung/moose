/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

#include "ComputeElemAuxVarsThread.h"
#include "AuxiliarySystem.h"
#include "AuxKernel.h"
#include "FEProblem.h"
// libmesh includes
#include "libmesh/threads.h"


ComputeElemAuxVarsThread::ComputeElemAuxVarsThread(FEProblem & problem, AuxiliarySystem & sys, const MooseObjectWarehouse<AuxKernel> & storage, bool need_materials) :
    ThreadedElementLoop<ConstElemRange>(problem, sys),
    _aux_sys(sys),
    _storage(storage),
    _need_materials(need_materials)
{
}

// Splitting Constructor
ComputeElemAuxVarsThread::ComputeElemAuxVarsThread(ComputeElemAuxVarsThread & x, Threads::split /*split*/) :
    ThreadedElementLoop<ConstElemRange>(x._fe_problem, x._system),
    _aux_sys(x._aux_sys),
    _storage(x._storage),
    _need_materials(x._need_materials)
{
}

ComputeElemAuxVarsThread::~ComputeElemAuxVarsThread()
{
}

void
ComputeElemAuxVarsThread::subdomainChanged()
{
  // prepare variables
  for (std::map<std::string, MooseVariable *>::iterator it = _aux_sys._elem_vars[_tid].begin(); it != _aux_sys._elem_vars[_tid].end(); ++it)
  {
    MooseVariable * var = it->second;
    var->prepareAux();
  }

  std::set<MooseVariable *> needed_moose_vars;

  if (_storage.hasActiveBlockObjects(_subdomain, _tid))
  {
    const std::vector<MooseSharedPointer<AuxKernel> > & kernels = _storage.getActiveBlockObjects(_subdomain, _tid);
    for (std::vector<MooseSharedPointer<AuxKernel> >::const_iterator aux_it = kernels.begin(); aux_it != kernels.end(); ++aux_it)
    {
      (*aux_it)->subdomainSetup();
      const std::set<MooseVariable *> & mv_deps = (*aux_it)->getMooseVariableDependencies();
      needed_moose_vars.insert(mv_deps.begin(), mv_deps.end());
    }
  }

  _fe_problem.setActiveElementalMooseVariables(needed_moose_vars, _tid);
  _fe_problem.prepareMaterials(_subdomain, _tid);
}


void
ComputeElemAuxVarsThread::onElement(const Elem * elem)
{
  if (_storage.hasActiveBlockObjects(_subdomain, _tid))
  {
    const std::vector<MooseSharedPointer<AuxKernel> > & kernels = _storage.getActiveBlockObjects(_subdomain, _tid);
    _fe_problem.prepare(elem, _tid);
    _fe_problem.reinitElem(elem, _tid);

    if (_need_materials)
      _fe_problem.reinitMaterials(elem->subdomain_id(), _tid);

    for (std::vector<MooseSharedPointer<AuxKernel> >::const_iterator aux_it = kernels.begin(); aux_it != kernels.end(); ++aux_it)
      (*aux_it)->compute();

    if (_need_materials)
      _fe_problem.swapBackMaterials(_tid);

    // update the solution vector
    {
      Threads::spin_mutex::scoped_lock lock(Threads::spin_mtx);
      for (std::map<std::string, MooseVariable *>::iterator it = _aux_sys._elem_vars[_tid].begin(); it != _aux_sys._elem_vars[_tid].end(); ++it)
      {
        MooseVariable * var = it->second;
        var->insert(_system.solution());
      }
    }
  }
}

void
ComputeElemAuxVarsThread::post()
{
  _fe_problem.clearActiveElementalMooseVariables(_tid);
}

void
ComputeElemAuxVarsThread::join(const ComputeElemAuxVarsThread & /*y*/)
{
}
