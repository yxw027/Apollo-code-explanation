/******************************************************************************
 * Copyright 2019 The Apollo Authors. All Rights Reserved.
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

#pragma once

#include <memory>
#include <thread>

#include "modules/canbus/vehicle/vehicle_controller.h"

#include "modules/canbus/proto/canbus_conf.pb.h"
#include "modules/canbus/proto/chassis.pb.h"
#include "modules/canbus/proto/vehicle_parameter.pb.h"
#include "modules/common/proto/error_code.pb.h"
#include "modules/control/proto/control_cmd.pb.h"

#include "modules/canbus/vehicle/zhongyun/protocol/brake_control_a4.h"
#include "modules/canbus/vehicle/zhongyun/protocol/gear_control_a1.h"
#include "modules/canbus/vehicle/zhongyun/protocol/parking_control_a5.h"
#include "modules/canbus/vehicle/zhongyun/protocol/steering_control_a2.h"
#include "modules/canbus/vehicle/zhongyun/protocol/torque_control_a3.h"

namespace apollo {
namespace canbus {
namespace zhongyun {

class ZhongyunController final : public VehicleController {
 public:
  ZhongyunController() {}

  //描述：override保留字表示当前函数重写了基类的虚函数。
  //目的：1.在函数比较多的情况下可以提示读者某个函数重写了基类虚函数（表示这个虚函数是从基类继承，不是派生类自己定义的）；
        //2.强制编译器检查某个函数是否重写基类虚函数，如果没有则报错。
  virtual ~ZhongyunController();

  ::apollo::common::ErrorCode Init(
      const VehicleParameter& params,
      CanSender<::apollo::canbus::ChassisDetail>* const can_sender,
      MessageManager<::apollo::canbus::ChassisDetail>* const message_manager)
      override;

  bool Start() override;

  /**
   * @brief stop the vehicle controller.
   * 停止车辆控制器。
   */
  void Stop() override;

  /**
   * @brief calculate and return the chassis.
   * 计算并返回chassis
   * @returns a copy of chassis. Use copy here to avoid multi-thread issues.
   * 底盘信息的副本。使用副本来避免多线程问题
   */
  Chassis chassis() override;
  FRIEND_TEST(ZhongyunControllerTest, SetDrivingMode);
  FRIEND_TEST(ZhongyunControllerTest, Status);
  FRIEND_TEST(ZhongyunControllerTest, UpdateDrivingMode);

 private:
  // main logical function for operation the car enter or exit the auto driving
  //汽车进入或退出自动驾驶操作的主要逻辑功能
  void Emergency() override; //紧急情况
  ::apollo::common::ErrorCode EnableAutoMode() override; //可以自动驾驶
  ::apollo::common::ErrorCode DisableAutoMode() override; //不可以自动驾驶
  ::apollo::common::ErrorCode EnableSteeringOnlyMode() override; //只可以转向
  ::apollo::common::ErrorCode EnableSpeedOnlyMode() override; //只可以速度

  // NEUTRAL, REVERSE, DRIVE
  void Gear(Chassis::GearPosition state) override;

  // brake with new acceleration
  // acceleration:0.00~99.99, unit:
  // acceleration_spd: 60 ~ 100, suggest: 90
  void Brake(double acceleration) override;

  // drive with old acceleration
  // gas:0.00~99.99 unit:
  void Throttle(double throttle) override;

  // drive with acceleration/deceleration
  // acc:-7.0~5.0 unit:m/s^2
  void Acceleration(double acc) override;

  // steering with old angle speed
  // angle:-99.99~0.00~99.99, unit:, left:+, right:-
  void Steer(double angle) override;

  // steering with new angle speed
  // angle:-99.99~0.00~99.99, unit:, left:+, right:-
  // angle_spd:0.00~99.99, unit:deg/s
  void Steer(double angle, double angle_spd) override;

  // set Electrical Park Brake
  void SetEpbBreak(const ::apollo::control::ControlCommand& command) override;
  void SetBeam(const ::apollo::control::ControlCommand& command) override;
  void SetHorn(const ::apollo::control::ControlCommand& command) override;
  void SetTurningSignal(
      const ::apollo::control::ControlCommand& command) override;

  void ResetProtocol(); //重置协议数据
  bool CheckChassisError(); 

 private:
  void SecurityDogThreadFunc(); //安全预警
  virtual bool CheckResponse(const int32_t flags, bool need_wait);
  void set_chassis_error_mask(const int32_t mask);
  int32_t chassis_error_mask();
  Chassis::ErrorCode chassis_error_code();
  void set_chassis_error_code(const Chassis::ErrorCode& error_code);

 private:
  // control protocol
  //nullptr的类型为nullptr_t，能够隐式的转换为任何指针。所以用空指针就尽可能的使用nullptr。
  //一开始都赋予空指针
  Brakecontrola4* brake_control_a4_ = nullptr; 
  Gearcontrola1* gear_control_a1_ = nullptr;
  Parkingcontrola5* parking_control_a5_ = nullptr;
  Steeringcontrola2* steering_control_a2_ = nullptr;
  Torquecontrola3* torque_control_a3_ = nullptr;

  Chassis chassis_;
  std::unique_ptr<std::thread> thread_;
  bool is_chassis_error_ = false; //默认底盘没有故障

  std::mutex chassis_error_code_mutex_;
  Chassis::ErrorCode chassis_error_code_ = Chassis::NO_ERROR; //默认故障码为没有故障

  std::mutex chassis_mask_mutex_;
  int32_t chassis_error_mask_ = 0; //默认底盘错误掩码为0
};

}  // namespace zhongyun
}  // namespace canbus
}  // namespace apollo
