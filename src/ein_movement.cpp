
#include "ein_words.h"
#include "ein.h"


namespace ein_words {


WORD(AssumeBackScanningPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.backScanningPose;
  ms->pushWord("waitUntilAtCurrentPosition");
}
END_WORD
REGISTER_WORD(AssumeBackScanningPose)

WORD(WaitUntilAtCurrentPosition)
CODE(131154)    // capslock + r
virtual void execute(std::shared_ptr<MachineState> ms) {

  ms->config.currentMovementState = MOVING;
  ms->config.lastTrueEEPoseEEPose = ms->config.trueEEPoseEEPose;
  ms->config.lastMovementStateSet = ros::Time::now();

  ms->config.waitUntilAtCurrentPositionCounter = 0;
  double dx = (ms->config.currentEEPose.px - ms->config.trueEEPose.position.x);
  double dy = (ms->config.currentEEPose.py - ms->config.trueEEPose.position.y);
  double dz = (ms->config.currentEEPose.pz - ms->config.trueEEPose.position.z);
  double distance = dx*dx + dy*dy + dz*dz;
  
  double qx = (fabs(ms->config.currentEEPose.qx) - fabs(ms->config.trueEEPose.orientation.x));
  double qy = (fabs(ms->config.currentEEPose.qy) - fabs(ms->config.trueEEPose.orientation.y));
  double qz = (fabs(ms->config.currentEEPose.qz) - fabs(ms->config.trueEEPose.orientation.z));
  double qw = (fabs(ms->config.currentEEPose.qw) - fabs(ms->config.trueEEPose.orientation.w));
  double angleDistance = qx*qx + qy*qy + qz*qz + qw*qw;
  
  if ((distance > ms->config.w1GoThresh*ms->config.w1GoThresh) || (angleDistance > ms->config.w1AngleThresh*ms->config.w1AngleThresh)) {
    ms->pushWord("waitUntilAtCurrentPositionB"); 
    ms->config.endThisStackCollapse = 1;
    ms->config.shouldIDoIK = 1;
  } else {
    ms->config.endThisStackCollapse = 1;
  }
}
END_WORD
REGISTER_WORD(WaitUntilAtCurrentPosition)

WORD(WaitUntilAtCurrentPositionB)
virtual void execute(std::shared_ptr<MachineState> ms) {

  if ( (ms->config.currentMovementState == STOPPED) ||
       (ms->config.currentMovementState == BLOCKED) ) {

    if (ms->config.currentMovementState == STOPPED) {
      cout << "Warning: waitUntilAtCurrentPosition ms->config.currentMovementState = STOPPED, moving on." << endl;
      ms->config.endThisStackCollapse = ms->config.endCollapse;
    }
    if (ms->config.currentMovementState == BLOCKED) {
      cout << "Warning: waitUntilAtCurrentPosition ms->config.currentMovementState = BLOCKED, moving on." << endl;
      ms->config.endThisStackCollapse = ms->config.endCollapse;
    }
    
    ms->config.currentEEPose.pz = ms->config.trueEEPose.position.z + 0.001;
    cout << "  backing up just a little to dislodge, then waiting again." << endl;

    ms->pushWord("waitUntilAtCurrentPosition"); 
    return;
  }

  double dx = (ms->config.currentEEPose.px - ms->config.trueEEPose.position.x);
  double dy = (ms->config.currentEEPose.py - ms->config.trueEEPose.position.y);
  double dz = (ms->config.currentEEPose.pz - ms->config.trueEEPose.position.z);
  double distance = dx*dx + dy*dy + dz*dz;
  
  double qx = (fabs(ms->config.currentEEPose.qx) - fabs(ms->config.trueEEPose.orientation.x));
  double qy = (fabs(ms->config.currentEEPose.qy) - fabs(ms->config.trueEEPose.orientation.y));
  double qz = (fabs(ms->config.currentEEPose.qz) - fabs(ms->config.trueEEPose.orientation.z));
  double qw = (fabs(ms->config.currentEEPose.qw) - fabs(ms->config.trueEEPose.orientation.w));
  double angleDistance = qx*qx + qy*qy + qz*qz + qw*qw;
  
  if (ms->config.waitUntilAtCurrentPositionCounter < ms->config.waitUntilAtCurrentPositionCounterTimeout) {
    ms->config.waitUntilAtCurrentPositionCounter++;
    if ((distance > ms->config.w1GoThresh*ms->config.w1GoThresh) || (angleDistance > ms->config.w1AngleThresh*ms->config.w1AngleThresh)) {
      ms->pushWord("waitUntilAtCurrentPositionB"); 
      ms->config.endThisStackCollapse = 1;
      ms->config.shouldIDoIK = 1;
    } else {
      ms->config.endThisStackCollapse = ms->config.endCollapse;
    }
  } else {
    cout << "Warning: waitUntilAtCurrentPosition timed out, moving on." << endl;
    ms->config.endThisStackCollapse = 1;
  }
}
END_WORD
REGISTER_WORD(WaitUntilAtCurrentPositionB)

WORD(WaitUntilGripperNotMoving)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.waitUntilGripperNotMovingCounter = 0;
  ms->config.lastGripperCallbackRequest = ros::Time::now();
  ms->pushWord("waitUntilGripperNotMovingB"); 
  ms->config.endThisStackCollapse = 1;
}
END_WORD
REGISTER_WORD(WaitUntilGripperNotMoving)

WORD(WaitUntilGripperNotMovingB)
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.lastGripperCallbackRequest >= ms->config.lastGripperCallbackReceived) {
    ms->pushWord("waitUntilGripperNotMovingB"); 
  } else {
    ms->config.lastGripperCallbackRequest = ros::Time::now();
    if (ms->config.waitUntilGripperNotMovingCounter < ms->config.waitUntilGripperNotMovingTimeout) {
      if (ms->config.gripperMoving) {
	ms->config.waitUntilGripperNotMovingCounter++;
	ms->pushWord("waitUntilGripperNotMovingB"); 
      } else {
	ms->pushWord("waitUntilGripperNotMovingC"); 
	ms->config.waitUntilGripperNotMovingStamp = ros::Time::now();
	ms->config.waitUntilGripperNotMovingCounter = 0;
      }
    } else {
      cout << "Warning: waitUntilGripperNotMovingB timed out, moving on." << endl;
    }
  }
  ms->config.endThisStackCollapse = 1;
}
END_WORD
REGISTER_WORD(WaitUntilGripperNotMovingB)

WORD(WaitUntilGripperNotMovingC)
virtual void execute(std::shared_ptr<MachineState> ms) {
// waits until gripper has not been moving for gripperNotMovingConfirmTime
  if (ms->config.lastGripperCallbackRequest >= ms->config.lastGripperCallbackReceived) {
    ms->pushWord("waitUntilGripperNotMovingC"); 
  } else {
    ms->config.lastGripperCallbackRequest = ros::Time::now();
    if (ms->config.waitUntilGripperNotMovingCounter < ms->config.waitUntilGripperNotMovingTimeout) {
      ros::Duration deltaSinceUpdate = ms->config.gripperLastUpdated - ms->config.waitUntilGripperNotMovingStamp;
      if (deltaSinceUpdate.toSec() <= ms->config.gripperNotMovingConfirmTime) {
	ms->config.waitUntilGripperNotMovingCounter++;
	ms->pushWord("waitUntilGripperNotMovingC"); 
      }
    } else {
      cout << "Warning: waitUntilGripperNotMovingC timed out, moving on." << endl;
    }
  }
  ms->config.endThisStackCollapse = 1;
}
END_WORD
REGISTER_WORD(WaitUntilGripperNotMovingC)

WORD(PerturbPosition)
CODE(1048623)     // numlock + /
virtual void execute(std::shared_ptr<MachineState> ms) {
  double param_perturbScale = 0.05;//0.1;
  double noX = param_perturbScale * ((drand48() - 0.5) * 2.0);
  double noY = param_perturbScale * ((drand48() - 0.5) * 2.0);
  double noTheta = 3.1415926 * ((drand48() - 0.5) * 2.0);
  
  ms->config.currentEEPose.px += noX;
  ms->config.currentEEPose.py += noY;

  ms->config.currentEEDeltaRPY.pz += noTheta;
}
END_WORD
REGISTER_WORD(PerturbPosition)

WORD(OYDown)
CODE('w'+65504) 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEDeltaRPY.py -= ms->config.bDelta;
}
END_WORD
REGISTER_WORD(OYDown)

WORD(OYUp)
CODE('s'+65504) 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEDeltaRPY.py += ms->config.bDelta;
}
END_WORD
REGISTER_WORD(OYUp)

WORD(OZDown)
CODE('q'+65504) 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEDeltaRPY.pz -= ms->config.bDelta;
}
END_WORD
REGISTER_WORD(OZDown)

WORD(OZUp)
CODE('e'+65504) 
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "Changing pose. " << endl;
  ms->config.currentEEDeltaRPY.pz += ms->config.bDelta;
}
END_WORD
REGISTER_WORD(OZUp)

WORD(OXDown)
CODE('a'+65504) 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEDeltaRPY.px -= ms->config.bDelta;
}
END_WORD
REGISTER_WORD(OXDown)

WORD(OXUp)
CODE('d'+65504) 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEDeltaRPY.px += ms->config.bDelta;
}
END_WORD
REGISTER_WORD(OXUp)


WORD(PushCurrentPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  shared_ptr<EePoseWord> word = std::make_shared<EePoseWord>(ms->config.currentEEPose);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(PushCurrentPose)

WORD(PushTruePose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  shared_ptr<EePoseWord> word = std::make_shared<EePoseWord>(ms->config.trueEEPoseEEPose);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(PushTruePose)


WORD(SaveRegister1)
CODE(65568+1) // ! 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.eepReg1 = ms->config.currentEEPose;
}
END_WORD
REGISTER_WORD(SaveRegister1)

WORD(SaveRegister2)
CODE(65600) // @
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.eepReg2 = ms->config.currentEEPose;
}
END_WORD
REGISTER_WORD(SaveRegister2)

WORD(SaveRegister3)
CODE(65568+3) // # 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.eepReg3 = ms->config.currentEEPose;
}
END_WORD
REGISTER_WORD(SaveRegister3)

WORD(SaveRegister4)
CODE( 65568+4) // $ 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.eepReg4 = ms->config.currentEEPose;
}
END_WORD
REGISTER_WORD(SaveRegister4)


WORD(MoveToRegister)
virtual void execute(std::shared_ptr<MachineState> ms) {

  std::shared_ptr<Word> registerword = ms->popWord();

  int register_num = registerword->to_int();
  if (register_num == 1) {
    ms->config.currentEEPose = ms->config.eepReg1;
  } else if (register_num == 2) {
    ms->config.currentEEPose = ms->config.eepReg2;
  } else if (register_num == 3) {
    ms->config.currentEEPose = ms->config.eepReg3;
  } else if (register_num == 4) {
    ms->config.currentEEPose = ms->config.eepReg4;
  } else if (register_num == 5) {
    ms->config.currentEEPose = ms->config.eepReg5;
  } else if (register_num == 6) {
    ms->config.currentEEPose = ms->config.eepReg6;
  } else {
    cout << "Bad register number: " << registerword << " int: " << register_num << endl;
  }
}
END_WORD
REGISTER_WORD(MoveToRegister)

WORD(MoveToRegister1)
CODE('1') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.eepReg1;
}
END_WORD
REGISTER_WORD(MoveToRegister1)

WORD(MoveToRegister2)
CODE('2') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.eepReg2;
}
END_WORD
REGISTER_WORD(MoveToRegister2)

WORD(MoveToRegister3)
CODE('3') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.eepReg3;
}
END_WORD
REGISTER_WORD(MoveToRegister3)

WORD(MoveToRegister4)
CODE('4') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.eepReg4;
}
END_WORD
REGISTER_WORD(MoveToRegister4)

WORD(MoveToRegister5)
CODE('5') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.eepReg5;
}
END_WORD
REGISTER_WORD(MoveToRegister5)


WORD(MoveToRegister6)
CODE('6') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.eepReg6;
}
END_WORD
REGISTER_WORD(MoveToRegister6)

WORD(LocalXDown)
virtual void execute(std::shared_ptr<MachineState> ms) {
  Vector3d localUnitX;
  Vector3d localUnitY;
  Vector3d localUnitZ;
  fillLocalUnitBasis(ms->config.trueEEPoseEEPose, &localUnitX, &localUnitY, &localUnitZ);
  ms->config.currentEEPose = ms->config.currentEEPose.minusP(ms->config.bDelta * localUnitX);
}
END_WORD
REGISTER_WORD(LocalXDown)


WORD(LocalXUp)
virtual void execute(std::shared_ptr<MachineState> ms) {
  Vector3d localUnitX;
  Vector3d localUnitY;
  Vector3d localUnitZ;
  fillLocalUnitBasis(ms->config.trueEEPoseEEPose, &localUnitX, &localUnitY, &localUnitZ);
  ms->config.currentEEPose = ms->config.currentEEPose.plusP(ms->config.bDelta * localUnitX);
}
END_WORD
REGISTER_WORD(LocalXUp)

WORD(LocalYDown)
virtual void execute(std::shared_ptr<MachineState> ms) {
  Vector3d localUnitX;
  Vector3d localUnitY;
  Vector3d localUnitZ;
  fillLocalUnitBasis(ms->config.trueEEPoseEEPose, &localUnitX, &localUnitY, &localUnitZ);
  ms->config.currentEEPose = ms->config.currentEEPose.minusP(ms->config.bDelta * localUnitY);
}
END_WORD
REGISTER_WORD(LocalYDown)


WORD(LocalYUp)
virtual void execute(std::shared_ptr<MachineState> ms) {
  Vector3d localUnitX;
  Vector3d localUnitY;
  Vector3d localUnitZ;
  fillLocalUnitBasis(ms->config.trueEEPoseEEPose, &localUnitX, &localUnitY, &localUnitZ);
  ms->config.currentEEPose = ms->config.currentEEPose.plusP(ms->config.bDelta * localUnitY);
}
END_WORD
REGISTER_WORD(LocalYUp)


WORD(LocalZUp)
virtual void execute(std::shared_ptr<MachineState> ms)
{
  Vector3d localUnitX;
  Vector3d localUnitY;
  Vector3d localUnitZ;
  fillLocalUnitBasis(ms->config.trueEEPoseEEPose, &localUnitX, &localUnitY, &localUnitZ);
  ms->config.currentEEPose = ms->config.currentEEPose.plusP(ms->config.bDelta * localUnitZ);
}
END_WORD
REGISTER_WORD(LocalZUp)

WORD(LocalZDown)
virtual void execute(std::shared_ptr<MachineState> ms)
{
  Vector3d localUnitX;
  Vector3d localUnitY;
  Vector3d localUnitZ;
  fillLocalUnitBasis(ms->config.trueEEPoseEEPose, &localUnitX, &localUnitY, &localUnitZ);
  ms->config.currentEEPose = ms->config.currentEEPose.minusP(ms->config.bDelta * localUnitZ);
}
END_WORD
REGISTER_WORD(LocalZDown)

WORD(XDown)
CODE('q') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose.px -= ms->config.bDelta;
}
END_WORD
REGISTER_WORD(XDown)


WORD(XUp)
CODE('e') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose.px += ms->config.bDelta;
}
END_WORD
REGISTER_WORD(XUp)

WORD(YDown)
CODE('a') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose.py -= ms->config.bDelta;
}
END_WORD
REGISTER_WORD(YDown)


WORD(YUp)
CODE('d') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose.py += ms->config.bDelta;
}
END_WORD
REGISTER_WORD(YUp)


WORD(ZUp)
CODE('w')
virtual void execute(std::shared_ptr<MachineState> ms)
{
  ms->config.currentEEPose.pz += ms->config.bDelta;
}
END_WORD
REGISTER_WORD(ZUp)

WORD(ZDown)
CODE('s')
virtual void execute(std::shared_ptr<MachineState> ms)
{
    ms->config.currentEEPose.pz -= ms->config.bDelta;
}
END_WORD
REGISTER_WORD(ZDown)

WORD(SetGripperThresh)
CODE(1179713)     // capslock + numlock + a
virtual void execute(std::shared_ptr<MachineState> ms) {
  double param_lastMeasuredBias = 1;
  ms->config.gripperThresh = ms->config.lastMeasuredClosed + param_lastMeasuredBias;
  cout << "lastMeasuredClosed: " << ms->config.lastMeasuredClosed << " lastMeasuredBias: " << param_lastMeasuredBias << endl;
  cout << "gripperThresh = " << ms->config.gripperThresh << endl;
}
END_WORD
REGISTER_WORD(SetGripperThresh)

WORD(CalibrateGripper)
CODE('i') 
virtual void execute(std::shared_ptr<MachineState> ms) {
  //baxter_core_msgs::EndEffectorCommand command;
  //command.command = baxter_core_msgs::EndEffectorCommand::CMD_CALIBRATE;
  //command.id = 65538;
  //ms->config.gripperPub.publish(command);
  calibrateGripper(ms);
}
END_WORD
REGISTER_WORD(CalibrateGripper)

WORD(CloseGripper)
CODE('j')
virtual void execute(std::shared_ptr<MachineState> ms) {
  baxter_core_msgs::EndEffectorCommand command;
  command.command = baxter_core_msgs::EndEffectorCommand::CMD_GO;
  command.args = "{\"position\": 0.0}";
  command.id = 65538;
  ms->config.gripperPub.publish(command);
}
END_WORD
REGISTER_WORD(CloseGripper)

WORD(OpenGripper)
CODE('k')
virtual void execute(std::shared_ptr<MachineState> ms) {
  baxter_core_msgs::EndEffectorCommand command;
  command.command = baxter_core_msgs::EndEffectorCommand::CMD_GO;
  command.args = "{\"position\": 100.0}";
  command.id = 65538;
  ms->config.gripperPub.publish(command);
  ms->config.lastMeasuredClosed = ms->config.gripperPosition;
}
END_WORD
REGISTER_WORD(OpenGripper)


WORD(SetMovementSpeedNowThatsFast)
CODE(1114193)    // numlock + Q
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.bDelta = NOW_THATS_FAST;
}
END_WORD
REGISTER_WORD(SetMovementSpeedNowThatsFast)

WORD(SetMovementSpeedMoveEvenFaster)
CODE(1114199)     // numlock + W
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.bDelta = MOVE_EVEN_FASTER;
}
END_WORD
REGISTER_WORD(SetMovementSpeedMoveEvenFaster)


WORD(SetMovementSpeedMoveFaster)
CODE(1114181)  // numlock + E
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.bDelta = MOVE_FASTER;
}
END_WORD
REGISTER_WORD(SetMovementSpeedMoveFaster)

WORD(SetMovementSpeedMoveFast)
CODE(1048674)     // numlock + b
virtual void execute(std::shared_ptr<MachineState> ms)  {
  ms->config.bDelta = MOVE_FAST;
}
END_WORD
REGISTER_WORD(SetMovementSpeedMoveFast)

WORD(SetMovementSpeedMoveMedium)
CODE(1048686)   // numlock + n
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.bDelta = MOVE_MEDIUM;
}
END_WORD
REGISTER_WORD(SetMovementSpeedMoveMedium)

WORD(SetMovementSpeedMoveSlow)
CODE(1114190) // numlock + N
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.bDelta = MOVE_SLOW;
}
END_WORD
REGISTER_WORD(SetMovementSpeedMoveSlow)

WORD(SetMovementSpeedMoveVerySlow)
CODE(1114178) // numlock + B
virtual void execute(std::shared_ptr<MachineState> ms) {
	ms->config.bDelta = MOVE_VERY_SLOW;
}
END_WORD
REGISTER_WORD(SetMovementSpeedMoveVerySlow)

WORD(ChangeToHeight0)
CODE(1245217) // capslock + numlock + !
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentThompsonHeightIdx = 0;
  ms->config.currentThompsonHeight = convertHeightIdxToGlobalZ(ms, ms->config.currentThompsonHeightIdx);
  ms->config.currentEEPose.pz = ms->config.currentThompsonHeight;
  // ATTN 23
  ms->config.reticle = ms->config.vanishingPointReticle;
  //ms->config.reticle = heightReticles[ms->config.currentThompsonHeightIdx];
  ms->config.m_x = ms->config.m_x_h[ms->config.currentThompsonHeightIdx];
  ms->config.m_y = ms->config.m_y_h[ms->config.currentThompsonHeightIdx];
}
END_WORD
REGISTER_WORD(ChangeToHeight0)

WORD(ChangeToHeight1)
CODE(1245248)     // capslock + numlock + @
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentThompsonHeightIdx = 1;
  ms->config.currentThompsonHeight = convertHeightIdxToGlobalZ(ms, ms->config.currentThompsonHeightIdx);
  ms->config.currentEEPose.pz = ms->config.currentThompsonHeight;
  // ATTN 23
  ms->config.reticle = ms->config.vanishingPointReticle;
  //ms->config.reticle = heightReticles[ms->config.currentThompsonHeightIdx];
  ms->config.m_x = ms->config.m_x_h[ms->config.currentThompsonHeightIdx];
  ms->config.m_y = ms->config.m_y_h[ms->config.currentThompsonHeightIdx];
}
END_WORD
REGISTER_WORD(ChangeToHeight1)

WORD(ChangeToHeight2)
CODE(1245219)  // capslock + numlock + #
virtual void execute(std::shared_ptr<MachineState> ms)  {
  ms->config.currentThompsonHeightIdx = 2;
  ms->config.currentThompsonHeight = convertHeightIdxToGlobalZ(ms, ms->config.currentThompsonHeightIdx);
  ms->config.currentEEPose.pz = ms->config.currentThompsonHeight;
  // ATTN 23
  ms->config.reticle = ms->config.vanishingPointReticle;
  //ms->config.reticle = heightReticles[ms->config.currentThompsonHeightIdx];
  ms->config.m_x = ms->config.m_x_h[ms->config.currentThompsonHeightIdx];
  ms->config.m_y = ms->config.m_y_h[ms->config.currentThompsonHeightIdx];
}
END_WORD
REGISTER_WORD(ChangeToHeight2)

WORD(ChangeToHeight3)
CODE(1245220) // capslock + numlock + $
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentThompsonHeightIdx = 3;
  ms->config.currentThompsonHeight = convertHeightIdxToGlobalZ(ms, ms->config.currentThompsonHeightIdx);
  ms->config.currentEEPose.pz = ms->config.currentThompsonHeight;
  // ATTN 23
  ms->config.reticle = ms->config.vanishingPointReticle;
  //ms->config.reticle = heightReticles[ms->config.currentThompsonHeightIdx];
  ms->config.m_x = ms->config.m_x_h[ms->config.currentThompsonHeightIdx];
  ms->config.m_y = ms->config.m_y_h[ms->config.currentThompsonHeightIdx];
}
END_WORD
REGISTER_WORD(ChangeToHeight3)

WORD(HundredthImpulse)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 0.01;
}
END_WORD
REGISTER_WORD(HundredthImpulse)

WORD(TenthImpulse)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 0.1;
}
END_WORD
REGISTER_WORD(TenthImpulse)

WORD(QuarterImpulse)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 0.25;
}
END_WORD
REGISTER_WORD(QuarterImpulse)

WORD(HalfImpulse)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 0.5;
}
END_WORD
REGISTER_WORD(HalfImpulse)

WORD(FullImpulse)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 1.0;
}
END_WORD
REGISTER_WORD(FullImpulse)

WORD(CruisingSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  //w1GoThresh = 0.40;
  //ms->config.currentEESpeedRatio = 0.75;
  ms->config.currentEESpeedRatio = 1.0;
}
END_WORD
REGISTER_WORD(CruisingSpeed)

WORD(ApproachSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  //w1GoThresh = 0.01;
  ms->config.currentEESpeedRatio = 0.05;//0.035;//0.07;//0.05;
}
END_WORD
REGISTER_WORD(ApproachSpeed)

WORD(DepartureSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  //ms->config.currentEESpeedRatio = 0.5;
  ms->config.currentEESpeedRatio = 0.05;
}
END_WORD
REGISTER_WORD(DepartureSpeed)

WORD(ResetW1ThreshToDefault)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.w1GoThresh = 0.03;
}
END_WORD
REGISTER_WORD(ResetW1ThreshToDefault)

WORD(RasterScanningSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  //w1GoThresh = 0.05;
  ms->config.currentEESpeedRatio = 0.05;//0.02;
}
END_WORD
REGISTER_WORD(RasterScanningSpeed)

WORD(FasterRasterScanningSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 0.1;
}
END_WORD
REGISTER_WORD(FasterRasterScanningSpeed)

WORD(IRCalibrationSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEESpeedRatio = 0.04;
}
END_WORD
REGISTER_WORD(IRCalibrationSpeed)

WORD(Hover)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.lastHoverTrueEEPoseEEPose = ms->config.trueEEPoseEEPose;
  ms->pushWord("hoverA");
  ms->config.endThisStackCollapse = 1;
  ms->config.shouldIDoIK = 1;
  ms->config.lastHoverRequest = ros::Time::now();
  ms->config.lastEndpointCallbackRequest = ms->config.lastHoverRequest;
}
END_WORD
REGISTER_WORD(Hover)

WORD(HoverA)
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.lastEndpointCallbackRequest >= ms->config.lastEndpointCallbackReceived) {
    ms->pushWord("hoverA");
    cout << "hoverA waiting for endpointCallback." << endl;
    ms->config.endThisStackCollapse = 1;
  } else {
    double dx = (ms->config.lastHoverTrueEEPoseEEPose.px - ms->config.trueEEPoseEEPose.px);
    double dy = (ms->config.lastHoverTrueEEPoseEEPose.py - ms->config.trueEEPoseEEPose.py);
    double dz = (ms->config.lastHoverTrueEEPoseEEPose.pz - ms->config.trueEEPoseEEPose.pz);
    double distance = dx*dx + dy*dy + dz*dz;
    
    double qx = (fabs(ms->config.lastHoverTrueEEPoseEEPose.qx) - fabs(ms->config.trueEEPoseEEPose.qx));
    double qy = (fabs(ms->config.lastHoverTrueEEPoseEEPose.qy) - fabs(ms->config.trueEEPoseEEPose.qy));
    double qz = (fabs(ms->config.lastHoverTrueEEPoseEEPose.qz) - fabs(ms->config.trueEEPoseEEPose.qz));
    double qw = (fabs(ms->config.lastHoverTrueEEPoseEEPose.qw) - fabs(ms->config.trueEEPoseEEPose.qw));
    double angleDistance = qx*qx + qy*qy + qz*qz + qw*qw;
  
    if ( ros::Time::now() - ms->config.lastHoverRequest < ros::Duration(ms->config.hoverTimeout) ) {
      if ((distance > ms->config.hoverGoThresh*ms->config.hoverGoThresh) || (angleDistance > ms->config.hoverAngleThresh*ms->config.hoverAngleThresh)) {
	ms->pushWord("hoverA"); 
	ms->config.endThisStackCollapse = 1;
	ms->config.shouldIDoIK = 1;
	cout << "hoverA distance requirement not met, distance angleDistance: " << distance << " " << angleDistance << endl;
	ms->config.lastHoverTrueEEPoseEEPose = ms->config.trueEEPoseEEPose;
      } else {
	ms->config.endThisStackCollapse = ms->config.endCollapse;
      }
    } else {
      cout << "Warning: hover timed out, moving on." << endl;
      ms->config.endThisStackCollapse = ms->config.endCollapse;
    }
  }
}
END_WORD
REGISTER_WORD(HoverA)

WORD(SpawnTargetClassAtEndEffector)
CODE(65379) // insert
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "SpawnTargetClassAtEndEffector called." << endl;
  if (ms->config.targetClass < 0) {
    cout << "Not spawning because targetClass is " << ms->config.targetClass << endl;
    return;
  }

  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    BoxMemory box;
    box.bTop.x = ms->config.vanishingPointReticle.px-ms->config.simulatedObjectHalfWidthPixels;
    box.bTop.y = ms->config.vanishingPointReticle.py-ms->config.simulatedObjectHalfWidthPixels;
    box.bBot.x = ms->config.vanishingPointReticle.px+ms->config.simulatedObjectHalfWidthPixels;
    box.bBot.y = ms->config.vanishingPointReticle.py+ms->config.simulatedObjectHalfWidthPixels;
    box.cameraPose = ms->config.currentEEPose;
    box.top = pixelToGlobalEEPose(ms, box.bTop.x, box.bTop.y, ms->config.trueEEPose.position.z + ms->config.currentTableZ);
    box.bot = pixelToGlobalEEPose(ms, box.bBot.x, box.bBot.y, ms->config.trueEEPose.position.z + ms->config.currentTableZ);
    box.centroid.px = (box.top.px + box.bot.px) * 0.5;
    box.centroid.py = (box.top.py + box.bot.py) * 0.5;
    box.centroid.pz = (box.top.pz + box.bot.pz) * 0.5;
    box.cameraTime = ros::Time::now();
    box.labeledClassIndex = ms->config.targetClass;
  
    mapBox(ms, box);
    vector<BoxMemory> newMemories;
    for (int i = 0; i < ms->config.blueBoxMemories.size(); i++) {
      newMemories.push_back(ms->config.blueBoxMemories[i]);
    }
    newMemories.push_back(box);
    ms->config.blueBoxMemories = newMemories;
  } else {
    assert(0);
  }
}
END_WORD
REGISTER_WORD(SpawnTargetClassAtEndEffector)

WORD(DestroyObjectInEndEffector)
CODE(65535) // delete
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.objectInHandLabel >= 0) {
    cout << "destroyObjectInEndEffector: The " << ms->config.classLabels[ms->config.objectInHandLabel] << " in your hand simply vanished." << endl;
    ms->config.objectInHandLabel = -1;
  } else {
    cout << "destroyObjectInEndEffector: There is nothing in your hand so there is nothing to destroy." << ms->config.objectInHandLabel << endl;
  }
}
END_WORD
REGISTER_WORD(DestroyObjectInEndEffector)

WORD(PickObjectUnderEndEffector)
CODE(65365) // page up
virtual void execute(std::shared_ptr<MachineState> ms) {
  std_msgs::Empty msg;
  pickObjectUnderEndEffectorCommandCallback(msg);
}
END_WORD
REGISTER_WORD(PickObjectUnderEndEffector)

WORD(PlaceObjectInEndEffector)
CODE(65366) // page down
virtual void execute(std::shared_ptr<MachineState> ms) {
  std_msgs::Empty msg;
  placeObjectInEndEffectorCommandCallback(msg);
}
END_WORD
REGISTER_WORD(PlaceObjectInEndEffector)

WORD(SetCurrentCornellTableToZero)
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "Setting currentCornellTableIndex to " << "0" << " out of " << ms->config.numCornellTables << "." << endl;
  ms->config.currentCornellTableIndex = 0;
}
END_WORD
REGISTER_WORD(SetCurrentCornellTableToZero)

WORD(IncrementCurrentCornellTable)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentCornellTableIndex = (ms->config.currentCornellTableIndex + 1 + ms->config.numCornellTables) % ms->config.numCornellTables;
  cout << "Incrementing currentCornellTableIndex to " << ms->config.currentCornellTableIndex << " out of " << ms->config.numCornellTables << "." << endl;
}
END_WORD
REGISTER_WORD(IncrementCurrentCornellTable)

WORD(DecrementCurrentCornellTable)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentCornellTableIndex = (ms->config.currentCornellTableIndex - 1 + ms->config.numCornellTables) % ms->config.numCornellTables;
  cout << "Decrementing currentCornellTableIndex to " << ms->config.currentCornellTableIndex << " out of " << ms->config.numCornellTables << "." << endl;
}
END_WORD
REGISTER_WORD(DecrementCurrentCornellTable)

WORD(MoveToCurrentCornellTable)
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.currentCornellTableIndex >= 0) {
    ms->config.currentEEPose.px = ms->config.cornellTables[ms->config.currentCornellTableIndex].px;
    ms->config.currentEEPose.py = ms->config.cornellTables[ms->config.currentCornellTableIndex].py;
  }
}
END_WORD
REGISTER_WORD(MoveToCurrentCornellTable)

WORD(SpawnTargetMasterSpriteAtEndEffector)
CODE(130915) // shift + insert
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "SpawnTargetMasterSpriteAtEndEffector called." << endl;
  if (ms->config.targetMasterSprite < 0) {
    cout << "Not spawning because targetMasterSprite is " << ms->config.targetMasterSprite << endl;
    return;
  }

  for (int s = 0; s < ms->config.masterSprites.size(); s++) {
    cout << "checked " << ms->config.masterSprites[s].name << " as masterSprites[" << s << "] scale " << ms->config.masterSprites[s].scale << " image size " << ms->config.masterSprites[s].image.size() << endl;
  }
  
  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    Sprite sprite;
    sprite.image = ms->config.masterSprites[ms->config.targetMasterSprite].image.clone();
    sprite.name = ms->config.masterSprites[ms->config.targetMasterSprite].name;
    sprite.scale = ms->config.masterSprites[ms->config.targetMasterSprite].scale;
    sprite.creationTime = ros::Time::now();
    sprite.pose = ms->config.currentEEPose;
    sprite.top = sprite.pose;
    sprite.bot = sprite.pose;

    // rotate image and grow size
    {
      Size sz = sprite.image.size();
      int deltaWidth = 0;
      int deltaHeight = 0;
      if (sz.width < sz.height) {
	deltaWidth = sz.height - sz.width;
	sz.width = sz.height;
      }
      if (sz.width > sz.height) {
	deltaHeight = sz.width - sz.height;
	sz.height = sz.width;
      }

      Quaternionf eeqform(sprite.pose.qw, sprite.pose.qx, sprite.pose.qy, sprite.pose.qz);
      Quaternionf crane2Orient(0, 1, 0, 0);
      Quaternionf rel = eeqform * crane2Orient.inverse();
      Quaternionf ex(0,1,0,0);
      Quaternionf zee(0,0,0,1);
	    
      Quaternionf result = rel * ex * rel.conjugate();
      Quaternionf thumb = rel * zee * rel.conjugate();
      double aY = result.y();
      double aX = result.x();

      double angle = vectorArcTan(ms, aY, aX)*180.0/3.1415926;
      angle = (angle);
      double scale = 1.0;
      Point center = Point(sz.width/2, sz.height/2);

      Mat un_rot_mat = getRotationMatrix2D( center, angle, scale );

      Mat trans_mat = getRotationMatrix2D( center, angle, scale );
      trans_mat.at<double>(0,0) = 1;
      trans_mat.at<double>(1,1) = 1;
      trans_mat.at<double>(0,1) = 0;
      trans_mat.at<double>(1,0) = 0;
      trans_mat.at<double>(0,2) = double(deltaWidth) / 2.0;
      trans_mat.at<double>(1,2) = double(deltaHeight) / 2.0;

      //Mat rotatedSpriteImage;
      //warpAffine(sprite.image, rotatedSpriteImage, un_rot_mat, sz, INTER_LINEAR, BORDER_REPLICATE);
      Mat translatedSpriteImage;
      warpAffine(sprite.image, translatedSpriteImage, trans_mat, sz, INTER_LINEAR, BORDER_REPLICATE);
      Mat rotatedSpriteImage;
      warpAffine(translatedSpriteImage, rotatedSpriteImage, un_rot_mat, sz, INTER_LINEAR, BORDER_REPLICATE);
      sprite.image = rotatedSpriteImage;
    }

    double halfWidthMeters = sprite.image.cols / (2.0 * sprite.scale);
    double halfHeightMeters = sprite.image.rows / (2.0 * sprite.scale);

    sprite.top.px -= halfWidthMeters;
    sprite.top.py -= halfHeightMeters;
    sprite.bot.px += halfWidthMeters;
    sprite.bot.py += halfHeightMeters;

    ms->config.instanceSprites.push_back(sprite);
  } else {
    assert(0);
  }
  cout << "Now instanceSprites.size() is " << ms->config.instanceSprites.size() << "." << endl;
}
END_WORD
REGISTER_WORD(SpawnTargetMasterSpriteAtEndEffector)

WORD(DestroyTargetInstanceSprite)
CODE(131071) // shift + delete
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "DestroyTargetInstanceSprite called." << endl;
  if ((ms->config.targetInstanceSprite < 0) ||
      (ms->config.targetInstanceSprite >= ms->config.instanceSprites.size())) {
    cout << "Not destoying because targetInstanceSprite is " << ms->config.targetInstanceSprite << " out of " << ms->config.instanceSprites.size() << endl;
    return;
  }
  
  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    vector<Sprite> newInstanceSprites;
    for (int s = 0; s < ms->config.instanceSprites.size(); s++) {
      if (s != ms->config.targetInstanceSprite) {
	newInstanceSprites.push_back(ms->config.instanceSprites[s]);
      }
    }
    ms->config.instanceSprites = newInstanceSprites;
  } else {
    assert(0);
  }
  cout << "Now instanceSprites.size() is " << ms->config.instanceSprites.size() << "." << endl;
}
END_WORD
REGISTER_WORD(DestroyTargetInstanceSprite)

WORD(IncrementTargetInstanceSprite)
CODE(130901) // shift + page up
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    int base = ms->config.instanceSprites.size();
    ms->config.targetInstanceSprite = (ms->config.targetInstanceSprite + 1 + base) % max(base, 1);
    cout << "Incrementing targetInstanceSprite to " << ms->config.targetInstanceSprite << " out of " << base << "." << endl;
  } else {
    assert(0);
  }
}
END_WORD
REGISTER_WORD(IncrementTargetInstanceSprite)

WORD(DecrementTargetInstanceSprite)
CODE(130902) // shift + page down
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    int base = ms->config.instanceSprites.size();
    ms->config.targetInstanceSprite = (ms->config.targetInstanceSprite - 1 + base) % max(base, 1);
    cout << "Decrementing targetInstanceSprite to " << ms->config.targetInstanceSprite << " out of " << base << "." << endl;
  } else {
    assert(0);
  }
}
END_WORD
REGISTER_WORD(DecrementTargetInstanceSprite)

WORD(IncrementTargetMasterSprite)
CODE(130896) // shift + home 
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    int base = ms->config.masterSprites.size();
    ms->config.targetMasterSprite = (ms->config.targetMasterSprite + 1 + base) % max(base, 1);
    cout << "Incrementing targetMasterSprite to " << ms->config.targetMasterSprite << " out of " << base << "." << endl;
  } else {
    assert(0);
  }
}
END_WORD
REGISTER_WORD(IncrementTargetMasterSprite)

WORD(DecrementTargetMasterSprite)
CODE(130903) // shift + end 
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.currentRobotMode == PHYSICAL) {
    return;
  } else if (ms->config.currentRobotMode == SIMULATED) {
    int base = ms->config.masterSprites.size();
    ms->config.targetMasterSprite = (ms->config.targetMasterSprite - 1 + base) % max(base, 1);
    cout << "Decrementing targetMasterSprite to " << ms->config.targetMasterSprite << " out of " << base << "." << endl;
  } else {
    assert(0);
  }
}
END_WORD
REGISTER_WORD(DecrementTargetMasterSprite)

WORD(ComeToStop)
virtual void execute(std::shared_ptr<MachineState> ms) {
  //ms->config.currentEEPose = ms->config.trueEEPoseEEPose;
  ms->pushWord("comeToStopA");
  ms->config.comeToStopStart = ros::Time::now();
  cout << "Waiting to come to a stop..." << endl;
}
END_WORD
REGISTER_WORD(ComeToStop)

WORD(ComeToStopA)
virtual void execute(std::shared_ptr<MachineState> ms) {
  if ( (ms->config.currentMovementState == STOPPED) ||
       (ms->config.currentMovementState == BLOCKED) ) {
    // do nothing
    cout << "Came to a stop, moving on." << endl;
  } else {
    ros::Duration timeSinceCTS = ros::Time::now() - ms->config.comeToStopStart;
    if (timeSinceCTS.toSec() < ms->config.comeToStopTimeout) {
      ms->pushWord("comeToStopA");
    } else {
      ROS_WARN_STREAM("_____*____*________");
      ROS_ERROR_STREAM("comeToStop timeout reached, moving on.");
      ROS_WARN_STREAM("_____*____*________");

      // waitUntilCurrentPosition will time out, make sure that there will
      //  be no cycles introduced
      ms->config.currentEEPose.pz = ms->config.trueEEPose.position.z + 0.001;
      cout << "  backing up just a little to dislodge from failed hover, then waiting." << endl;
      ms->pushWord("waitUntilAtCurrentPosition"); 
    }
    ms->config.endThisStackCollapse = 1;
  }
}
END_WORD
REGISTER_WORD(ComeToStopA)

WORD(ComeToHover)
virtual void execute(std::shared_ptr<MachineState> ms) {
  //ms->config.currentEEPose = ms->config.trueEEPoseEEPose;
  ms->pushWord("comeToHoverA");
  ms->config.comeToHoverStart = ros::Time::now();
  cout << "Waiting to come to a hover..." << endl;
}
END_WORD
REGISTER_WORD(ComeToHover)

WORD(ComeToHoverA)
virtual void execute(std::shared_ptr<MachineState> ms) {
  if ( (ms->config.currentMovementState == HOVERING) ) {
    // do nothing
    cout << "Came to a hover, moving on." << endl;
  } else {
    ros::Duration timeSinceCTH = ros::Time::now() - ms->config.comeToHoverStart;
    if (timeSinceCTH.toSec() < ms->config.comeToHoverTimeout) {
      ms->pushWord("comeToHoverA");
    } else {
      ROS_WARN_STREAM("_____*____*________");
      ROS_ERROR_STREAM("comeToHover timeout reached, moving on.");
      ROS_WARN_STREAM("_____*____*________");
    }
    ms->config.endThisStackCollapse = 1;
  }
}
END_WORD
REGISTER_WORD(ComeToHoverA)

WORD(WaitForTugThenOpenGripper)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->pushWord("waitForTugThenOpenGripperA");
  ms->pushWord("waitUntilEndpointCallbackReceived");
  ms->pushWord("comeToHover");
  ms->pushWord("waitUntilAtCurrentPosition");
  ms->config.waitForTugStart = ros::Time::now();
  cout << "Waiting to feel a tug... " << ARMED << " " << ms->config.currentMovementState << endl;
}
END_WORD
REGISTER_WORD(WaitForTugThenOpenGripper)

WORD(WaitForTugThenOpenGripperA)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.endThisStackCollapse = 1;
  if (0) { // position based
    if ( ( ms->config.currentMovementState == MOVING ) ||
	 ( !ms->config.gripperGripping ) ) {
      if ( !ms->config.gripperGripping ) {
	cout << "There is nothing in the gripper so we should move on..." << endl;
      }
      if (ms->config.currentMovementState == MOVING) {
	cout << "Felt a tug, opening gripper." << endl;
      }
      ms->pushWord("openGripper");
    } else {
      ms->config.currentMovementState = ARMED;
      ros::Duration timeSinceWFT = ros::Time::now() - ms->config.waitForTugStart;
      if (timeSinceWFT.toSec() < ms->config.waitForTugTimeout) {
	ms->pushWord("waitForTugThenOpenGripperA");
      } else {
	ROS_WARN_STREAM("_____*____*________");
	ROS_ERROR_STREAM("waitForTugThenOpenGripper timeout reached, moving on.");
	ROS_WARN_STREAM("_____*____*________");
      }
    }
  } else { // wrench based
    double wrenchNorm = sqrt( eePose::squareDistance(eePose::zero(), ms->config.trueEEWrench) );
    double wrenchThresh = 15;
    bool wrenchOverThresh = ( wrenchNorm > wrenchThresh );
    if ( wrenchOverThresh ||
	 ( !ms->config.gripperGripping ) ) {
      if ( !ms->config.gripperGripping ) {
	cout << "There is nothing in the gripper so we should move on..." << endl;
      }
      if ( wrenchOverThresh ) {
	cout << "Felt a tug, opening gripper; wrenchNorm wrenchThresh: " << wrenchNorm << " " << wrenchThresh << " " << endl;
      }
      ms->pushWord("openGripper");
    } else {
      ms->config.currentMovementState = ARMED;
      ros::Duration timeSinceWFT = ros::Time::now() - ms->config.waitForTugStart;
      if (timeSinceWFT.toSec() < ms->config.waitForTugTimeout) {
	ms->pushWord("waitForTugThenOpenGripperA");
      } else {
	ROS_WARN_STREAM("_____*____*________");
	ROS_ERROR_STREAM("waitForTugThenOpenGripper timeout reached, moving on.");
	ROS_WARN_STREAM("_____*____*________");
      }
    }
  }
}
END_WORD
REGISTER_WORD(WaitForTugThenOpenGripperA)

WORD(Idler)
virtual void execute(std::shared_ptr<MachineState> ms) {
  if (ms->config.currentIdleMode == EMPTY) {
    // empty
  } else if (ms->config.currentIdleMode == STOPCLEAR) {
    ms->pushWord("clearStack"); 
  } else if (ms->config.currentIdleMode == PATROL) {
    ms->pushWord("clearStackIntoMappingPatrol"); 
  } else if (ms->config.currentIdleMode == CRANE) {
    ms->pushWord("clearStackAcceptFetchCommands"); 
    ms->pushWord("publishRecognizedObjectArrayFromBlueBoxMemory");
    ms->pushWord("assumeCrane1"); 
  } else if (ms->config.currentIdleMode == SHRUG) {
    ms->pushWord("clearStackAcceptFetchCommands"); 
    ms->pushWord("publishRecognizedObjectArrayFromBlueBoxMemory");
    ms->pushWord("assumeShrugPose"); 
  } else {
    assert(0);
  }
  ms->pushWord("setPatrolStateToIdling");
}
END_WORD
REGISTER_WORD(Idler)

WORD(SetMovementStateToMoving)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentMovementState = MOVING;
  ms->config.lastTrueEEPoseEEPose = ms->config.trueEEPoseEEPose;
  ms->config.lastMovementStateSet = ros::Time::now();
}
END_WORD
REGISTER_WORD(SetMovementStateToMoving)

WORD(AssumeCrane1)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.crane1;
  ms->pushWord("waitUntilAtCurrentPosition");
}
END_WORD
REGISTER_WORD(AssumeCrane1)

WORD(AssumeBeeHome)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.beeHome;
  ms->pushWord("waitUntilAtCurrentPosition");
}
END_WORD
REGISTER_WORD(AssumeBeeHome)

WORD(AssumeShrugPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.shrugPose;
  ms->pushWord("waitUntilAtCurrentPosition");
}
END_WORD
REGISTER_WORD(AssumeShrugPose)

WORD(AssumeHandingPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentEEPose = ms->config.handingPose;
  ms->pushWord("waitUntilAtCurrentPosition");
}
END_WORD
REGISTER_WORD(AssumeHandingPose)

WORD(SetPatrolStateToIdling)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolState = IDLING;
}
END_WORD
REGISTER_WORD(SetPatrolStateToIdling)

WORD(SetPatrolStateToPatrolling)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolState = PATROLLING;
}
END_WORD
REGISTER_WORD(SetPatrolStateToPatrolling)

WORD(SetPatrolStateToPicking)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolState = PICKING;
}
END_WORD
REGISTER_WORD(SetPatrolStateToPicking)

WORD(SetPatrolStateToPlacing)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolState = PLACING;
}
END_WORD
REGISTER_WORD(SetPatrolStateToPlacing)

WORD(SetPatrolStateToHanding)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolState = HANDING;
}
END_WORD
REGISTER_WORD(SetPatrolStateToHanding)

WORD(SetPatrolModeToLoop)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolMode = LOOP;
}
END_WORD
REGISTER_WORD(SetPatrolModeToLoop)

WORD(SetPatrolModeToOnce)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentPatrolMode = ONCE;
}
END_WORD
REGISTER_WORD(SetPatrolModeToOnce)

WORD(SetIdleModeToCrane)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentIdleMode = CRANE;
}
END_WORD
REGISTER_WORD(SetIdleModeToCrane)

WORD(SetIdleModeToShrug)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentIdleMode = SHRUG;
}
END_WORD
REGISTER_WORD(SetIdleModeToShrug)

WORD(SetIdleModeToEmpty)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentIdleMode = EMPTY;
}
END_WORD
REGISTER_WORD(SetIdleModeToEmpty)

WORD(SetIdleModeToStopClear)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentIdleMode = STOPCLEAR;
}
END_WORD
REGISTER_WORD(SetIdleModeToStopClear)

WORD(SetIdleModeToPatrol)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.currentIdleMode = PATROL;
}
END_WORD
REGISTER_WORD(SetIdleModeToPatrol)

WORD(SetCurrentPoseToTruePose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "Setting current position to true position." << endl;
  ms->config.endThisStackCollapse = 1;
  ms->config.currentEEPose = ms->config.trueEEPoseEEPose;
}
END_WORD
REGISTER_WORD(SetCurrentPoseToTruePose)

WORD(DislodgeEndEffectorFromTable)
virtual void execute(std::shared_ptr<MachineState> ms) {
  cout << "Dislodging end effector from table 1cm..." << endl;
  ms->pushWord("waitUntilAtCurrentPosition");
  ms->pushWord("zUp"); 
}
END_WORD
REGISTER_WORD(DislodgeEndEffectorFromTable)
	
WORD(MoveToEEPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
	// get 7 strings, make them into doubles, move there
	// order px py pz qx qy qz qw
	double vals[7]; 
	for (int i = 6; i >= 0; i--) { 
		shared_ptr<Word> word = ms->popWord(); 
		if (word == NULL) {
			// failed
  	  cout << "not enough words" << endl;
      return;
		}
		char* endptr;
		string wordname = word->to_string(); 
		double r = strtod(wordname.c_str(), &endptr); 
		if (endptr == wordname && r == 0) { 
			// failed to convert
  		cout << "received not a number" << endl;
			ms->clearStack(); 
      return;
		}
		vals[i] = r;
	}
  // make them actually into an eepose
  _eePose pose = {.px = vals[0], .py = vals[1], .pz = vals[2],
      .qx = vals[3], .qy = vals[4], .qz = vals[5], .qw = vals[6]};
  ms->config.currentEEPose = pose; 
}
END_WORD
REGISTER_WORD(MoveToEEPose)
		


}
