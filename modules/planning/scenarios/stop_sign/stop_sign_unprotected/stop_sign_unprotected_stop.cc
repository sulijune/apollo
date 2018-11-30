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

#include "modules/planning/scenarios/stop_sign/stop_sign_unprotected/stop_sign_unprotected_stop.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "modules/perception/proto/perception_obstacle.pb.h"

#include "cyber/common/log.h"
#include "modules/common/time/time.h"
#include "modules/common/vehicle_state/vehicle_state_provider.h"
#include "modules/planning/common/frame.h"
#include "modules/planning/common/planning_context.h"
#include "modules/planning/toolkits/deciders/decider_creep.h"

namespace apollo {
namespace planning {
namespace scenario {
namespace stop_sign {

using common::TrajectoryPoint;
using common::time::Clock;
using hdmap::HDMapUtil;
using hdmap::LaneInfo;
using hdmap::LaneInfoConstPtr;
using hdmap::OverlapInfoConstPtr;
using perception::PerceptionObstacle;

using StopSignLaneVehicles =
    std::unordered_map<std::string, std::vector<std::string>>;

Stage::StageStatus StopSignUnprotectedStop::Process(
    const TrajectoryPoint& planning_init_point, Frame* frame) {
  ADEBUG << "stage: Stop";
  CHECK_NOTNULL(frame);

  scenario_config_.CopyFrom(GetContext()->scenario_config);

  bool plan_ok = PlanningOnReferenceLine(planning_init_point, frame);
  if (!plan_ok) {
    AERROR << "StopSignUnprotectedPreStop planning error";
  }

  auto start_time = GetContext()->stop_start_time;
  const double wait_time = Clock::NowInSeconds() - start_time;
  ADEBUG << "stop_start_time[" << start_time
      << "] wait_time[" << wait_time << "]";
  auto& watch_vehicles = GetContext()->watch_vehicles;
  if (wait_time >= scenario_config_.stop_duration() &&
      watch_vehicles.empty()) {
    next_stage_ = ScenarioConfig::STOP_SIGN_UNPROTECTED_CREEP;
    PlanningContext::GetScenarioInfo()->stop_done_overlap_id =
        GetContext()->stop_sign_id;
    return Stage::FINISHED;
  }

  // get all vehicles currently watched
  std::vector<std::string> watch_vehicle_ids;
  for (auto it = watch_vehicles.begin(); it != watch_vehicles.end(); ++it) {
    std::copy(it->second.begin(), it->second.end(),
              std::back_inserter(watch_vehicle_ids));

    // for debug
    std::string associated_lane_id = it->first;
    std::string s;
    for (size_t i = 0; i < watch_vehicle_ids.size(); ++i) {
      std::string vehicle = watch_vehicle_ids[i];
      s = s.empty() ? vehicle : s + "," + vehicle;
    }
    ADEBUG << "watch_vehicles: lane_id[" << associated_lane_id
         << "] vehicle[" << s << "]";
  }

  if (watch_vehicle_ids.size() == 0) {
    next_stage_ = ScenarioConfig::STOP_SIGN_UNPROTECTED_CREEP;
    PlanningContext::GetScenarioInfo()->stop_done_overlap_id =
        GetContext()->stop_sign_id;
    return Stage::FINISHED;
  }

  // check timeout
  /* TODO(all): need revisit
   if (wait_time > scenario_config_.wait_timeout() &&
      watch_vehicle_ids.size() <= 1) {
    next_stage_ = ScenarioConfig::STOP_SIGN_UNPROTECTED_CREEP;
    PlanningContext::GetScenarioInfo()->stop_done_overlap_id =
        GetContext()->stop_sign_id;
    return Stage::FINISHED;
  }
  */

  const auto& reference_line_info = frame->reference_line_info().front();
  const PathDecision& path_decision = reference_line_info.path_decision();
  for (const auto* obstacle : path_decision.obstacles().Items()) {
    // remove from watch_vehicles_ if adc is stopping/waiting at stop sign
    RemoveWatchVehicle(*obstacle, watch_vehicle_ids, &watch_vehicles);
  }

  return Stage::RUNNING;
}

/**
 * @brief: remove a watch vehicle which not stopping at stop sign any more
 */
int StopSignUnprotectedStop::RemoveWatchVehicle(
    const Obstacle& obstacle,
    const std::vector<std::string>& watch_vehicle_ids,
    StopSignLaneVehicles* watch_vehicles) {
  CHECK_NOTNULL(watch_vehicles);

  const PerceptionObstacle& perception_obstacle = obstacle.Perception();
  const std::string& obstacle_id = std::to_string(perception_obstacle.id());
  PerceptionObstacle::Type obstacle_type = perception_obstacle.type();
  std::string obstacle_type_name = PerceptionObstacle_Type_Name(obstacle_type);

  // check type
  if (obstacle_type != PerceptionObstacle::UNKNOWN &&
      obstacle_type != PerceptionObstacle::UNKNOWN_MOVABLE &&
      obstacle_type != PerceptionObstacle::BICYCLE &&
      obstacle_type != PerceptionObstacle::VEHICLE) {
    ADEBUG << "obstacle_id[" << obstacle_id << "] type[" << obstacle_type_name
           << "]. skip";
    return 0;
  }

  // check if being watched
  if (std::find(watch_vehicle_ids.begin(), watch_vehicle_ids.end(),
                obstacle_id) == watch_vehicle_ids.end()) {
    ADEBUG << "obstacle_id[" << obstacle_id << "] type[" << obstacle_type_name
           << "] not being watched. skip";
    return 0;
  }

  for (auto it = watch_vehicles->begin(); it != watch_vehicles->end(); ++it) {
    if (std::find(it->second.begin(), it->second.end(),
                  obstacle_id) == it->second.end()) {
      continue;
    }

    std::string associated_lane_id = it->first;
    auto assoc_lane_it = std::find_if(
        GetContext()->associated_lanes.begin(),
        GetContext()->associated_lanes.end(),
        [&associated_lane_id](
            std::pair<LaneInfoConstPtr, OverlapInfoConstPtr>& assc_lane) {
          return assc_lane.first.get()->id().id() == associated_lane_id;
        });
    if (assoc_lane_it == GetContext()->associated_lanes.end()) {
      continue;
    }

    auto stop_sign_over_lap_info =
        assoc_lane_it->second.get()->GetObjectOverlapInfo(
            hdmap::MakeMapId(associated_lane_id));
    if (stop_sign_over_lap_info == nullptr) {
      AERROR << "can't find stop_sign_over_lap_info for id: "
          << associated_lane_id;
      continue;
    }
    const double stop_line_end_s =
        stop_sign_over_lap_info->lane_overlap_info().end_s();

    const auto lane = HDMapUtil::BaseMap().GetLaneById(
        hdmap::MakeMapId(associated_lane_id));
    if (lane == nullptr) {
      continue;
    }
    auto stop_sign_point = lane.get()->GetSmoothPoint(stop_line_end_s);
    auto obstacle_point = common::util::MakePointENU(
        perception_obstacle.position().x(),
        perception_obstacle.position().y(),
        perception_obstacle.position().z());

    double distance = common::util::DistanceXY(
        stop_sign_point, obstacle_point);
    AERROR << "obstacle_id[" << obstacle_id
        << "] distance[" << distance << "]";

    // TODO(all): move 10.0 to conf
    if (distance > 10.0) {
      ADEBUG << "ERASE obstacle_id[" << obstacle_id << "]";
      for (StopSignLaneVehicles::iterator it = watch_vehicles->begin();
           it != watch_vehicles->end(); ++it) {
        std::vector<std::string>& vehicles = it->second;
        vehicles.erase(std::remove(vehicles.begin(), vehicles.end(),
                                   obstacle_id),
                                   vehicles.end());
      }
      return 0;
    }
  }

  return 0;
}

}  // namespace stop_sign
}  // namespace scenario
}  // namespace planning
}  // namespace apollo
