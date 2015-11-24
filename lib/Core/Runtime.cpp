/*                                                                 --*- C++ -*-
  Copyright (C) 2012 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  Runtime.cpp -- implementation of Runtime class
 */
#include <unistd.h>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include "Support/MachineConfig.h"
#include "Support/Thread.h"
#include "Core/Runtime.h"
#include "Core/Program.h"
#include "Core/AbstractPartitioner.h"
#include "Core/AbstractScheduler.h"
#include "Core/AbstractUberKernel.h"
#include "Core/UberKernelFactory.h"
#include "Core/ComputeDeviceFactory.h"
#include "Core/SystemContext.h"
#include "DebugInfo/EventLogger.h"

using namespace danbi; 

Runtime::Runtime()
  : Pgm(NULL), Partitioner(NULL), Scheduler(NULL) 
{
  // do nothing
}

Runtime::~Runtime() {
  for (std::map<int,AbstractUberKernel*>::iterator
         i = Uberkernels.begin(), e = Uberkernels.end(); 
       i != e; ++i) {
    delete i->second; 
  }
  delete Scheduler; 
  delete Partitioner; 
  delete Pgm; 
}

int Runtime::loadProgram(Program* Pgm_, 
                         AbstractPartitioner* Partitioner_, 
                         AbstractScheduler* Scheduler_) {
  // check precondition
  assert(Pgm_ != NULL && "Program shoud be non-null."); 
  assert(Partitioner_ != NULL && "Partitioner shoud be non-null."); 
  assert(Scheduler_ != NULL && "Scheduler shoud be non-null."); 

  // set
  Pgm = Pgm_; 
  Partitioner = Partitioner_; 
  Scheduler = Scheduler_; 
  SystemContext::DDScheduler = reinterpret_cast<DistributedDynamicScheduler*>(Scheduler_); 

  // verify the program
  if (!Pgm->verify())
    return -EINVAL; 

  int ret; 
  // compute device assignment
  ret = Partitioner->partition(); 
  if (ret)
    return ret; 

  // build scheduling information 
  ret = Scheduler->buildSchedulingInfo(); 
  if (ret)
    return ret; 

  // build per-device uber kernel 
  ret = buildPerDeviceUberKernel();
  if (ret)
    return ret; 

  // create compute devices
  ret = createComputeDevices(); 
  if (ret)
    return ret; 

  // initialize logging
  ret = EventLogger::initialize(Pgm->getNumberOfKernels(), 
                                Partitioner->getMaxAssignableCores(
                                  AbstractPartitioner::CPU_DEVICE_INDEX)); 
  return ret; 
}

int Runtime::buildPerDeviceUberKernel() {
  // check precondition
  assert(Pgm != NULL && "Program shoud be non-null."); 
  assert(Partitioner != NULL && "Partitioner shoud be non-null."); 
  assert(Scheduler != NULL && "Scheduler shoud be non-null."); 

  // For each kernel
  for (std::vector<Kernel*>::iterator 
         i = Pgm->KernelTable.begin(), e = Pgm->KernelTable.end(); 
       i != e; ++i) {
    // Get assigned device id of the kernel 
    int DeviceID = Partitioner->getComputeDevice( (*i)->getIndex() ); 
    
    // Get an uber-kernel for the device
    AbstractUberKernel* Uberkernel = NULL; 
    std::map<int,AbstractUberKernel*>::iterator 
      UberkernelIter = Uberkernels.find(DeviceID); 
    if (UberkernelIter == Uberkernels.end()) {
      Uberkernel = UberKernelFactory::newUberKernel(DeviceID, Scheduler); 
      if (!Uberkernel)
        return -ENOMEM; 
      Uberkernels[DeviceID] = Uberkernel; 
    }
    else
      Uberkernel = UberkernelIter->second; 
    assert( Uberkernel != NULL ); 
    
    // Add this kernel to the uber kernel 
    Uberkernel->addKernel(**i);
  }

  // For each uber kernel, uberkernelize and load to the device. 
  for (std::map<int,AbstractUberKernel*>::iterator
         i = Uberkernels.begin(), e = Uberkernels.end(); 
       i != e; ++i) {
    int ret;
    ret = i->second->uberkernelize(); 
    if (!ret) return ret; 
    ret = i->second->loadToDevice(i->first); 
    if (!ret) return ret; 
  }

  // For each 
  return 0; 
}

int Runtime::createComputeDevices() {
  // create compute device for each device id
  for (std::map<int,AbstractUberKernel*>::iterator
         i = Uberkernels.begin(), e = Uberkernels.end(); 
       i != e; ++i) {
    int DevID = i->first; 
    AbstractUberKernel* UberKernel = i->second; 
    AbstractComputeDevice* Device 
      = ComputeDeviceFactory::newComputeDevice(DevID, 
                                               Partitioner->getMaxAssignableCores(DevID), 
                                               Pgm->KernelTable.size(), 
                                               *Scheduler, 
                                               *UberKernel); 
    if (!Device)
      return -ENOMEM; 
    ComputeDevices[DevID] = Device; 
  }
  return 0; 
}

int Runtime::prepare() {
  for (std::map<int,AbstractComputeDevice*>::iterator
         i = ComputeDevices.begin(), e = ComputeDevices.end(); 
       i != e; ++i) {
    int ret = i->second->prepare(); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int Runtime::start() {
  for (std::map<int,AbstractComputeDevice*>::iterator
         i = ComputeDevices.begin(), e = ComputeDevices.end(); 
       i != e; ++i) {
    int ret = i->second->start(); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int Runtime::join() {
  for (std::map<int,AbstractComputeDevice*>::iterator
         i = ComputeDevices.begin(), e = ComputeDevices.end(); 
       i != e; ++i) {
    int ret = i->second->join(); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int Runtime::generateLog(std::string SimpleGraphPath, std::string FullGraphPath) {
  return EventLogger::generateGraph(SimpleGraphPath, FullGraphPath); 
}

