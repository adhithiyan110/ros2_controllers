// Copyright 2014, SRI International
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// \author Sachin Chitta, Adolfo Rodriguez Tsouroukdissian

#ifndef PARALLEL_GRIPPER_CONTROLLER__PARALLEL_GRIPPER_ACTION_CONTROLLER_HPP_
#define PARALLEL_GRIPPER_CONTROLLER__PARALLEL_GRIPPER_ACTION_CONTROLLER_HPP_

// C++ standard
#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

// ROS
#include "rclcpp/rclcpp.hpp"

// ROS messages
#include "control_msgs/action/parallel_gripper_command.hpp"

// rclcpp_action
#include "rclcpp_action/create_server.hpp"

// ros_controls
#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/loaned_command_interface.hpp"
#include "hardware_interface/loaned_state_interface.hpp"
#include "realtime_tools/realtime_server_goal_handle.hpp"
#include "realtime_tools/realtime_thread_safe_box.hpp"

// Project
#include "parallel_gripper_controller/parallel_gripper_action_controller_parameters.hpp"

namespace parallel_gripper_action_controller
{
/**
 * \brief Controller for executing a `ParallelGripperCommand` action.
 *
 * \tparam HardwareInterface Controller hardware interface. Currently \p
 * hardware_interface::HW_IF_POSITION and \p
 * hardware_interface::HW_IF_EFFORT are supported out-of-the-box.
 */

class GripperActionController : public controller_interface::ControllerInterface
{
public:
  /**
   * \brief Store position and max effort in struct to allow easier realtime
   * buffer usage
   */
  struct Commands
  {
    double position_cmd_;  // Commanded position
    double max_velocity_;  // Desired max gripper velocity
    double max_effort_;    // Desired max allowed effort
  };
  GripperActionController();

  /**
   * @brief command_interface_configuration This controller requires the
   * position command interfaces for the controlled joints
   */
  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  /**
   * @brief command_interface_configuration This controller requires the
   * position and velocity state interfaces for the controlled joints
   */
  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

protected:
  using GripperCommandAction = control_msgs::action::ParallelGripperCommand;
  using ActionServer = rclcpp_action::Server<GripperCommandAction>;
  using ActionServerPtr = ActionServer::SharedPtr;
  using GoalHandle = rclcpp_action::ServerGoalHandle<GripperCommandAction>;
  using RealtimeGoalHandle =
    realtime_tools::RealtimeServerGoalHandle<control_msgs::action::ParallelGripperCommand>;
  using RealtimeGoalHandlePtr = std::shared_ptr<RealtimeGoalHandle>;
  using RealtimeGoalHandleBox = realtime_tools::RealtimeThreadSafeBox<RealtimeGoalHandlePtr>;

  // the realtime container to exchange the reference from subscriber
  realtime_tools::RealtimeThreadSafeBox<Commands> command_;
  // pre-allocated memory that is reused
  Commands command_struct_;
  // save the last reference in case of unable to get value from box
  Commands command_struct_rt_;

  std::string name_;  ///< Controller name.
  std::optional<std::reference_wrapper<hardware_interface::LoanedCommandInterface>>
    joint_command_interface_;
  std::optional<std::reference_wrapper<hardware_interface::LoanedCommandInterface>>
    effort_interface_;
  std::optional<std::reference_wrapper<hardware_interface::LoanedCommandInterface>>
    speed_interface_;
  std::optional<std::reference_wrapper<hardware_interface::LoanedStateInterface>>
    joint_position_state_interface_;
  std::optional<std::reference_wrapper<hardware_interface::LoanedStateInterface>>
    joint_velocity_state_interface_;

  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  RealtimeGoalHandleBox
    rt_active_goal_;  ///< Container for the currently active action goal, if any.
  control_msgs::action::ParallelGripperCommand::Result::SharedPtr pre_alloc_result_;

  rclcpp::Duration action_monitor_period_;

  // ROS API
  ActionServerPtr action_server_;

  rclcpp::TimerBase::SharedPtr goal_handle_timer_;

  rclcpp_action::GoalResponse goal_callback(
    const rclcpp_action::GoalUUID & uuid, std::shared_ptr<const GripperCommandAction::Goal> goal);

  rclcpp_action::CancelResponse cancel_callback(const std::shared_ptr<GoalHandle> goal_handle);

  void accepted_callback(std::shared_ptr<GoalHandle> goal_handle);

  void preempt_active_goal();

  void set_hold_position();

  rclcpp::Time last_movement_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);  ///< Store stall time
  double computed_command_;                                             ///< Computed command

  /**
   * \brief Check for success and publish appropriate result and feedback.
   **/
  void check_for_success(
    const rclcpp::Time & time, double error_position, double current_position,
    double current_velocity);
};

}  // namespace parallel_gripper_action_controller

#include "parallel_gripper_controller/parallel_gripper_action_controller_impl.hpp"

#endif  // PARALLEL_GRIPPER_CONTROLLER__PARALLEL_GRIPPER_ACTION_CONTROLLER_HPP_
