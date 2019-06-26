/******************************************************************************
Copyright (c) 2017, Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

 * Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include "ocs2_cart_pole_example/CartPoleInterface.h"
#include <ros/package.h>

namespace ocs2 {
namespace cartpole {

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
CartPoleInterface::CartPoleInterface(const std::string& taskFileFolderName)
{
	taskFile_ = ros::package::getPath("ocs2_cart_pole_example") + "/config/" + taskFileFolderName + "/task.info";
	std::cerr << "Loading task file: " << taskFile_ << std::endl;

	libraryFolder_ = ros::package::getPath("ocs2_cart_pole_example") + "/auto_generated";
	std::cerr << "Generated library path: " << libraryFolder_ << std::endl;

	// load setting from loading file
	loadSettings(taskFile_);

	// MPC
	setupOptimizer(taskFile_);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void CartPoleInterface::loadSettings(const std::string& taskFile) {

	/*
	 * Default initial condition
	 */
	loadEigenMatrix(taskFile, "initialState", initialState_);

	/*
	 * SLQ-MPC settings
	 */
	slqSettings_.loadSettings(taskFile);
	mpcSettings_.loadSettings(taskFile);

	/*
	 * Cartpole parameters
	 */
	CartPoleParameters<dim_t::scalar_t> cartPoleParameters;
	cartPoleParameters.loadSettings(taskFile);

	/*
	 * Dynamics
	 */
	cartPoleSystemDynamicsPtr_.reset(new CartPoleSytemDynamics(cartPoleParameters));
	cartPoleSystemDynamicsPtr_->createModels("cartpole_dynamics", libraryFolder_);

	/*
	 * Cost function
	 */
	loadEigenMatrix(taskFile, "Q", Q_);
	loadEigenMatrix(taskFile, "R", R_);
	loadEigenMatrix(taskFile, "Q_final", QFinal_);
	loadEigenMatrix(taskFile, "x_final", xFinal_);
	//	xNominal_ = dim_t::state_vector_t::Zero();
	xNominal_ = xFinal_;
	uNominal_ = dim_t::input_vector_t::Zero();

	std::cerr << "Q:  \n" << Q_ << std::endl;
	std::cerr << "R:  \n" << R_ << std::endl;
	std::cerr << "Q_final:\n" << QFinal_ << std::endl;
	std::cerr << "x_init:   "   << initialState_.transpose() << std::endl;
	std::cerr << "x_final:  "   << xFinal_.transpose() << std::endl;

	cartPoleCostPtr_.reset(new CartPoleCost(Q_, R_, xNominal_, uNominal_, QFinal_, xFinal_));

	/*
	 * Constraints
	 */
	cartPoleConstraintPtr_.reset(new CartPoleConstraint);

	/*
	 * Initialization
	 */
//	cartPoleOperatingPointPtr_.reset(new CartPoleOperatingPoint(
//			dim_t::state_vector_t::Zero(), dim_t::input_vector_t::Zero()));
	cartPoleOperatingPointPtr_.reset(new CartPoleOperatingPoint(
			initialState_, dim_t::input_vector_t::Zero()));

	/*
	 * Time partitioning which defines the time horizon and the number of data partitioning
	 */
	scalar_t timeHorizon;
	definePartitioningTimes(taskFile, timeHorizon,
			numPartitions_, partitioningTimes_, true);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void CartPoleInterface::setupOptimizer(const std::string& taskFile) {

	mpcPtr_.reset(new mpc_t(
			cartPoleSystemDynamicsPtr_.get(),
			cartPoleSystemDynamicsPtr_.get(),
			cartPoleConstraintPtr_.get(),
			cartPoleCostPtr_.get(),
			cartPoleOperatingPointPtr_.get(),
			partitioningTimes_,
			slqSettings_,
			mpcSettings_));
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
CartPoleInterface::mpc_t::Ptr& CartPoleInterface::getMPCPtr() {

	return mpcPtr_;
}

} // namespace cartpole
} // namespace ocs2

