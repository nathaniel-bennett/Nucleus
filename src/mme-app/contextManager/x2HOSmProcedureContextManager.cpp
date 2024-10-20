 /*
 * Copyright 2021-present Infosys Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */ 
/******************************************************************************
 * x2HOSmProcedureContextManager.cpp
 * This is an auto generated file.
 * Please do not edit this file.
 * All edits to be made through template source file
 * <TOP-DIR/scripts/SMCodeGen/templates/ctxtManagerTmpls/blockPoolManager.cpp.tt>
 ******************************************************************************/

#include "memPoolManager.h"
#include "contextManager/dataBlocks.h"
#include "contextManager/x2HOSmProcedureContextManager.h"

using namespace cmn::memPool;

namespace mme
{
	/******************************************************************************
	* Constructor
	******************************************************************************/
	X2HOSmProcedureContextManager::X2HOSmProcedureContextManager(int numOfBlocks):poolManager_m(numOfBlocks)
	{
	}
	
	/******************************************************************************
	* Destructor
	******************************************************************************/
	X2HOSmProcedureContextManager::~X2HOSmProcedureContextManager()
	{
	}
	
	/******************************************************************************
	* Allocate X2HOSmProcedureContext data block
	******************************************************************************/
	X2HOSmProcedureContext* X2HOSmProcedureContextManager::allocateX2HOSmProcedureContext()
	{
		X2HOSmProcedureContext* X2HOSmProcedureContext_p = poolManager_m.allocate();
		return X2HOSmProcedureContext_p;
	}
	
	/******************************************************************************
	* Deallocate a X2HOSmProcedureContext data block
	******************************************************************************/
	void X2HOSmProcedureContextManager::deallocateX2HOSmProcedureContext(X2HOSmProcedureContext* X2HOSmProcedureContextp )
	{
		poolManager_m.free( X2HOSmProcedureContextp );
	}
}