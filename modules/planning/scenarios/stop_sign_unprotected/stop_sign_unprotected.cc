/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 **/

#include "modules/planning/scenarios/stop_sign_unprotected/stop_sign_unprotected.h"  // NOINT


#include "modules/planning/proto/planning_config.pb.h"

#include "cybertron/common/log.h"
#include "modules/common/util/file.h"
#include "modules/planning/common/frame.h"
#include "modules/planning/common/planning_gflags.h"
#include "modules/planning/toolkits/deciders/decider_creep.h"
#include "modules/planning/toolkits/optimizers/dp_poly_path/dp_poly_path_optimizer.h"
#include "modules/planning/toolkits/optimizers/dp_st_speed/dp_st_speed_optimizer.h"
#include "modules/planning/toolkits/optimizers/path_decider/path_decider.h"
#include "modules/planning/toolkits/optimizers/qp_spline_path/qp_spline_path_optimizer.h"
#include "modules/planning/toolkits/optimizers/qp_spline_st_speed/qp_spline_st_speed_optimizer.h"
#include "modules/planning/toolkits/optimizers/speed_decider/speed_decider.h"

namespace apollo {
namespace planning {

using common::ErrorCode;
using common::Status;
using common::TrajectoryPoint;

int StopSignUnprotectedScenario::current_stage_index_ = 0;

void StopSignUnprotectedScenario::RegisterTasks() {
  // deciders
  task_factory_.Register(DECIDER_CREEP,
                         []() -> Task* { return new DeciderCreep(); });
  // optimizers
  task_factory_.Register(DP_POLY_PATH_OPTIMIZER,
                         []() -> Task* { return new DpPolyPathOptimizer(); });
  task_factory_.Register(PATH_DECIDER,
                         []() -> Task* { return new PathDecider(); });
  task_factory_.Register(DP_ST_SPEED_OPTIMIZER,
                         []() -> Task* { return new DpStSpeedOptimizer(); });
  task_factory_.Register(SPEED_DECIDER,
                         []() -> Task* { return new SpeedDecider(); });
  task_factory_.Register(QP_SPLINE_ST_SPEED_OPTIMIZER, []() -> Task* {
    return new QpSplineStSpeedOptimizer();
  });
}

bool StopSignUnprotectedScenario::Init() {
  if (is_init_) {
    return true;
  }

  CHECK(apollo::common::util::GetProtoFromFile(
      FLAGS_scenario_stop_sign_unprotected_config_file, &config_));

  RegisterTasks();

  is_init_ = true;
  status_ = STATUS_INITED;

  return true;
}

Status StopSignUnprotectedScenario::Process(
    const TrajectoryPoint& planning_start_point,
    Frame* frame) {
  status_ = STATUS_PROCESSING;

  if (!InitTasks(config_, current_stage_index_, &tasks_)) {
    return Status(ErrorCode::PLANNING_ERROR, "failed to init tasks");
  }

  // TODO(all)

  Status status = Status(ErrorCode::PLANNING_ERROR,
                         "Failed to process stage in stop_sign_upprotected.");
  switch (stage_) {
    case StopSignUnprotectedStage::CRUISE_AND_MONITOR: {
      status = CruiseAndMonitor(frame);
      break;
    }
    case StopSignUnprotectedStage::STOP: {
      status = Stop(frame);
      break;
    }
    case StopSignUnprotectedStage::CREEP: {
      status = Creep(planning_start_point, frame);
      break;
    }
    case StopSignUnprotectedStage::INTERSECTION_CRUISE: {
      status = IntersectionCruise(planning_start_point, frame);
      break;
    }
    default:
      break;
  }

  return Status::OK();
}

bool StopSignUnprotectedScenario::IsTransferable(
    const Scenario& current_scenario,
    const common::TrajectoryPoint& ego_point,
    const Frame& frame) const {
  // TODO(All): implement here
  return false;
}

common::Status StopSignUnprotectedScenario::CruiseAndMonitor(
    Frame* frame) {
  // TODO(all)
  return Status::OK();
}

common::Status StopSignUnprotectedScenario::Stop(
    Frame* frame) {
  // TODO(all)
  return Status::OK();
}

common::Status StopSignUnprotectedScenario::Creep(
    const common::TrajectoryPoint& planning_start_point,
    Frame* frame) {
  // TODO(all)
  return Status::OK();
}

common::Status StopSignUnprotectedScenario::IntersectionCruise(
    const common::TrajectoryPoint& planning_start_point,
    Frame* frame) {
  // TODO(all)
  return Status::OK();
}

}  // namespace planning
}  // namespace apollo
