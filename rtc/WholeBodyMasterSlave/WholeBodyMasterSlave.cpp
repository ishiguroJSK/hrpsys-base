#include <rtm/CorbaNaming.h>
#include <hrpModel/Link.h>
#include <hrpModel/Sensor.h>
#include <hrpModel/ModelLoaderUtil.h>
#include "WholeBodyMasterSlave.h"
#include <hrpModel/JointPath.h>
#include <hrpUtil/MatrixSolvers.h>
#include "hrpsys/util/Hrpsys.h"

typedef coil::Guard<coil::Mutex> Guard;
using namespace rats;

static const char* WholeBodyMasterSlave_spec[] =
    {
        "implementation_id", "WholeBodyMasterSlave",
        "type_name",         "WholeBodyMasterSlave",
        "description",       "wholebodymasterslave component",
        "version",           HRPSYS_PACKAGE_VERSION,
        "vendor",            "AIST",
        "category",          "example",
        "activity_type",     "DataFlowComponent",
        "max_instance",      "10",
        "language",          "C++",
        "lang_type",         "compile",
        "conf.default.debugLevel", "0",
        ""
    };

WholeBodyMasterSlave::WholeBodyMasterSlave(RTC::Manager* manager)
    : RTC::DataFlowComponentBase(manager),
      // from sh
      m_qRefIn("qRef", m_qRef),
      m_zmpIn("zmpIn", m_zmp),
      m_basePosIn("basePosIn", m_basePos),
      m_baseRpyIn("baseRpyIn", m_baseRpy),
      m_optionalDataIn("optionalData", m_optionalData),
      // to abc
      m_qOut("q", m_qRef),
      m_zmpOut("zmpOut", m_zmp),
      m_basePosOut("basePosOut", m_basePos),
      m_baseRpyOut("baseRpyOut", m_baseRpy),
      m_optionalDataOut("optionalDataOut", m_optionalData),
      // from ros bridge
      m_htzmpIn("htzmpIn", m_htzmp),
      m_htrfwIn("htrfwIn", m_htrfw),
      m_htlfwIn("htlfwIn", m_htlfw),
      m_htcomIn("htcomIn", m_htcom),
      m_htrfIn("htrfIn", m_htrf),
      m_htlfIn("htlfIn", m_htlf),
      m_htrhIn("htrhIn", m_htrh),
      m_htlhIn("htlhIn", m_htlh),
//      m_actzmpIn("actzmpIn", m_actzmp),
      m_htheadIn("htheadIn", m_hthead),

#ifdef USE_DEBUG_PORT
      // to ros bridge
       m_htcom_dbgOut("htcom_dbgOut", m_htcom_dbg),
       m_htrf_dbgOut("htrf_dbgOut", m_htrf_dbg),
       m_htlf_dbgOut("htlf_dbgOut", m_htlf_dbg),
       m_htrh_dbgOut("htrh_dbgOut", m_htrh_dbg),
       m_htlh_dbgOut("htlh_dbgOut", m_htlh_dbg),
       m_hthead_dbgOut("hthead_dbgOut", m_hthead_dbg),
       m_htzmp_dbgOut("htzmp_dbgOut", m_htzmp_dbg),
       m_htrfw_dbgOut("htrfw_dbgOut", m_htrfw_dbg),
       m_htlfw_dbgOut("htlfw_dbgOut", m_htlfw_dbg),
       m_rpcom_dbgOut("rpcom_dbgOut", m_rpcom_dbg),
       m_rprf_dbgOut("rprf_dbgOut", m_rprf_dbg),
       m_rplf_dbgOut("rplf_dbgOut", m_rplf_dbg),
       m_rprh_dbgOut("rprh_dbgOut", m_rprh_dbg),
       m_rplh_dbgOut("rplh_dbgOut", m_rplh_dbg),
       m_rphead_dbgOut("rphead_dbgOut", m_rphead_dbg),
       m_rpzmp_dbgOut("rpzmp_dbgOut", m_rpzmp_dbg),
       m_rpdcp_dbgOut("rpdcp_dbgOut", m_rpdcp_dbg),
       m_rpacp_dbgOut("rpacp_dbgOut", m_rpacp_dbg),
       m_invdyn_dbgOut("invdyn_dbgOut", m_invdyn_dbg),
#endif
      m_WholeBodyMasterSlaveServicePort("WholeBodyMasterSlaveService"),
      m_robot(hrp::BodyPtr()),
      m_debugLevel(0)
{
    m_service0.wholebodymasterslave(this);
}

WholeBodyMasterSlave::~WholeBodyMasterSlave()
{
}

RTC::ReturnCode_t WholeBodyMasterSlave::onInitialize()
{
    std::cerr << "[" << m_profile.instance_name << "] onInitialize()" << std::endl;
    bindParameter("debugLevel", m_debugLevel, "0");

    addInPort("qRef", m_qRefIn);
    addInPort("zmpIn", m_zmpIn);
    addInPort("basePosIn", m_basePosIn);
    addInPort("baseRpyIn", m_baseRpyIn);
    addInPort("optionalData", m_optionalDataIn);

    addOutPort("q", m_qOut);
    addOutPort("zmpOut", m_zmpOut);
    addOutPort("basePosOut", m_basePosOut);
    addOutPort("baseRpyOut", m_baseRpyOut);
    addOutPort("optionalDataOut", m_optionalDataOut);

    addInPort("htzmpIn", m_htzmpIn);
    addInPort("htrfwIn", m_htrfwIn);
    addInPort("htlfwIn", m_htlfwIn);
    addInPort("htcomIn", m_htcomIn);
    addInPort("htrfIn", m_htrfIn);
    addInPort("htlfIn", m_htlfIn);
    addInPort("htrhIn", m_htrhIn);
    addInPort("htlhIn", m_htlhIn);
//    addInPort("actzmpIn", m_actzmpIn);
    addInPort("htheadIn", m_htheadIn);

#ifdef USE_DEBUG_PORT
     addOutPort("htcom_dbgOut", m_htcom_dbgOut);
     addOutPort("htrf_dbgOut", m_htrf_dbgOut);
     addOutPort("htlf_dbgOut", m_htlf_dbgOut);
     addOutPort("htrh_dbgOut", m_htrh_dbgOut);
     addOutPort("htlh_dbgOut", m_htlh_dbgOut);
     addOutPort("hthead_dbgOut", m_hthead_dbgOut);
     addOutPort("htzmp_dbgOut", m_htzmp_dbgOut);
     addOutPort("htrfw_dbgOut", m_htrfw_dbgOut);
     addOutPort("htlfw_dbgOut", m_htlfw_dbgOut);
     addOutPort("rpcom_dbgOut", m_rpcom_dbgOut);
     addOutPort("rprf_dbgOut", m_rprf_dbgOut);
     addOutPort("rplf_dbgOut", m_rplf_dbgOut);
     addOutPort("rprh_dbgOut", m_rprh_dbgOut);
     addOutPort("rplh_dbgOut", m_rplh_dbgOut);
     addOutPort("rphead_dbgOut", m_rphead_dbgOut);
     addOutPort("rpzmp_dbgOut", m_rpzmp_dbgOut);
     addOutPort("rpdcp_dbgOut", m_rpdcp_dbgOut);
     addOutPort("rpacp_dbgOut", m_rpacp_dbgOut);
     addOutPort("invdyn_dbgOut", m_invdyn_dbgOut);
#endif

    m_WholeBodyMasterSlaveServicePort.registerProvider("service0", "WholeBodyMasterSlaveService", m_service0);
  
    addPort(m_WholeBodyMasterSlaveServicePort);
  
    RTC::Properties& prop =  getProperties();
    coil::stringTo(m_dt, prop["dt"].c_str());

    m_robot = hrp::BodyPtr(new hrp::Body());
    m_robot_rmc = hrp::BodyPtr(new hrp::Body());
    RTC::Manager& rtcManager = RTC::Manager::instance();
    std::string nameServer = rtcManager.getConfig()["corba.nameservers"];
    int comPos = nameServer.find(",");
    if (comPos < 0){
        comPos = nameServer.length();
    }
    nameServer = nameServer.substr(0, comPos);
    RTC::CorbaNaming naming(rtcManager.getORB(), nameServer.c_str());
    if (!loadBodyFromModelLoader(m_robot, prop["model"].c_str(), CosNaming::NamingContext::_duplicate(naming.getRootContext()) )){
      std::cerr << "[" << m_profile.instance_name << "] failed to load model[" << prop["model"] << "]" << std::endl;
      return RTC::RTC_ERROR;
    }
    if (!loadBodyFromModelLoader(m_robot_rmc, prop["model"].c_str(), CosNaming::NamingContext::_duplicate(naming.getRootContext()) )){
      std::cerr << "[" << m_profile.instance_name << "] failed to load model 2 [" << prop["model"] << "]" << std::endl;
      return RTC::RTC_ERROR;
    }

    // allocate memory for outPorts
    m_qRef.data.length(m_robot->numJoints());
    m_htrfw.data.length(6);
    m_htlfw.data.length(6);
    loop = 0;

    q_interpolator = new interpolator(1, m_dt, interpolator::HOFFARBIB, 1);
    q_interpolator->setName(std::string(m_profile.instance_name)+" q_interpolator");
    q_interpolator_ratio = 0;
    double tmp_ratio = 0.0;
    q_interpolator->clear();
    q_interpolator->set(&tmp_ratio);
    tmp_ratio = 1.0;
    q_interpolator->setGoal(&tmp_ratio, 3.0, true);

//    // setting from conf file
//    // GaitGenerator requires abc_leg_offset and abc_stride_parameter in robot conf file
//    // setting leg_pos from conf file
//    coil::vstring leg_offset_str = coil::split(prop["abc_leg_offset"], ",");
//    leg_names.push_back("rleg");
//    leg_names.push_back("lleg");
//
    // Generate FIK
    fik = fikPtr(new SimpleFullbodyInverseKinematicsSolver(m_robot, std::string(m_profile.instance_name), m_dt));
    fik_rmc = fikPtr(new SimpleFullbodyInverseKinematicsSolver(m_robot_rmc, std::string(m_profile.instance_name), m_dt));

    setupfik(fik, m_robot, prop);
    setupfik(fik_rmc, m_robot_rmc, prop);

    if (fik->ikp.find("rleg") != fik->ikp.end() && fik->ikp.find("lleg") != fik->ikp.end()) {
      is_legged_robot = true;
    } else {
      is_legged_robot = false;
    }

    hsp = boost::shared_ptr<HumanSynchronizer>(new HumanSynchronizer(m_dt));

    return RTC::RTC_OK;
}


void WholeBodyMasterSlave::setupfik(fikPtr& fik_in, hrp::BodyPtr& robot_in, RTC::Properties& prop_in){
  // setting from conf file
  // rleg,TARGET_LINK,BASE_LINK
  coil::vstring end_effectors_str = coil::split(prop_in["end_effectors"], ",");
  size_t prop_num = 10;
  if (end_effectors_str.size() > 0) {
    size_t num = end_effectors_str.size()/prop_num;
    for (size_t i = 0; i < num; i++) {
      std::string ee_name, ee_target, ee_base;
      coil::stringTo(ee_name, end_effectors_str[i*prop_num].c_str());
      coil::stringTo(ee_target, end_effectors_str[i*prop_num+1].c_str());
      coil::stringTo(ee_base, end_effectors_str[i*prop_num+2].c_str());
      ABCIKparam tp;
      for (size_t j = 0; j < 3; j++) {
          coil::stringTo(tp.localPos(j), end_effectors_str[i*prop_num+3+j].c_str());
      }
      double tmpv[4];
      for (int j = 0; j < 4; j++ ) {
        coil::stringTo(tmpv[j], end_effectors_str[i*prop_num+6+j].c_str());
      }
      tp.localR = Eigen::AngleAxis<double>(tmpv[3], hrp::Vector3(tmpv[0], tmpv[1], tmpv[2])).toRotationMatrix(); // rotation in VRML is represented by axis + angle
      // FIK param
      SimpleFullbodyInverseKinematicsSolver::IKparam tmp_fikp;
      tmp_fikp.manip = hrp::JointPathExPtr(new hrp::JointPathEx(robot_in, robot_in->link(ee_base), robot_in->link(ee_target), m_dt, false, std::string(m_profile.instance_name)));
      tmp_fikp.target_link = robot_in->link(ee_target);
      tmp_fikp.localPos = tp.localPos;
      tmp_fikp.localR = tp.localR;
      fik_in->ikp.insert(std::pair<std::string, SimpleFullbodyInverseKinematicsSolver::IKparam>(ee_name, tmp_fikp));
      // Fix for toe joint
      //   Toe joint is defined as end-link joint in the case that end-effector link != force-sensor link
      //   Without toe joints, "end-effector link == force-sensor link" is assumed.
      //   With toe joints, "end-effector link != force-sensor link" is assumed.
      if (robot_in->link(ee_target)->sensors.size() == 0) { // If end-effector link has no force sensor
          std::vector<double> optw(fik_in->ikp[ee_name].manip->numJoints(), 1.0);
          optw.back() = 0.0; // Set weight = 0 for toe joint by default
          fik_in->ikp[ee_name].manip->setOptionalWeightVector(optw);
          tp.has_toe_joint = true;
      } else {
          tp.has_toe_joint = false;
      }
      tp.target_link = robot_in->link(ee_target);
//        ikp.insert(std::pair<std::string, ABCIKparam>(ee_name , tp));
//        ee_vec.push_back(ee_name);
      std::cerr << "[" << m_profile.instance_name << "] End Effector [" << ee_name << "]" << std::endl;
      std::cerr << "[" << m_profile.instance_name << "]   target = " << fik_in->ikp[ee_name].target_link->name << ", base = " << ee_base << std::endl;
      std::cerr << "[" << m_profile.instance_name << "]   offset_pos = " << tp.localPos.format(Eigen::IOFormat(Eigen::StreamPrecision, 0, ", ", ", ", "", "", "    [", "]")) << "[m]" << std::endl;
      std::cerr << "[" << m_profile.instance_name << "]   has_toe_joint = " << (tp.has_toe_joint?"true":"false") << std::endl;
      contact_states_index_map.insert(std::pair<std::string, size_t>(ee_name, i));////TODO:要移動
    }
  }

}

RTC::ReturnCode_t WholeBodyMasterSlave::onFinalize()
{
  return RTC::RTC_OK;
}

RTC::ReturnCode_t WholeBodyMasterSlave::onActivated(RTC::UniqueId ec_id)
{
    std::cerr << "[" << m_profile.instance_name<< "] onActivated(" << ec_id << ")" << std::endl;
    return RTC::RTC_OK;
}

RTC::ReturnCode_t WholeBodyMasterSlave::onDeactivated(RTC::UniqueId ec_id)
{
  std::cerr << "[" << m_profile.instance_name<< "] onDeactivated(" << ec_id << ")" << std::endl;
  return RTC::RTC_OK;
}

#define DEBUGP ((loop%200==0))
RTC::ReturnCode_t WholeBodyMasterSlave::onExecute(RTC::UniqueId ec_id)
{
//  std::cerr << "[" << m_profile.instance_name<< "] onExecute(" << ec_id << ")" << std::endl;
  // struct timeval t_calc_start, t_calc_end;
  // gettimeofday(&t_calc_start, NULL);
    if (m_qRefIn.isNew()) { m_qRefIn.read(); }
    if (m_basePosIn.isNew()) { m_basePosIn.read(); }
    if (m_baseRpyIn.isNew()) { m_baseRpyIn.read(); }
    if (m_zmpIn.isNew()) { m_zmpIn.read(); }
    if (m_optionalDataIn.isNew()) { m_optionalDataIn.read(); }
    //for HumanSynchronizer
    if (m_htrfwIn.isNew()){ m_htrfwIn.read(); HumanSynchronizer::DoubleSeqToWrench6(m_htrfw.data,hsp->hp_wld_raw.w[rfw]); }
    if (m_htlfwIn.isNew()){ m_htlfwIn.read(); HumanSynchronizer::DoubleSeqToWrench6(m_htlfw.data,hsp->hp_wld_raw.w[lfw]); }
    if (m_htcomIn.isNew()){ m_htcomIn.read(); HumanSynchronizer::Pose3DToHRPPose3D(m_htcom.data,hsp->hp_wld_raw.P[com]); }
    if (m_htrfIn.isNew()) { m_htrfIn.read();  HumanSynchronizer::Pose3DToHRPPose3D(m_htrf.data,hsp->hp_wld_raw.P[rf]); }
    if (m_htlfIn.isNew()) { m_htlfIn.read();  HumanSynchronizer::Pose3DToHRPPose3D(m_htlf.data,hsp->hp_wld_raw.P[lf]); }
    if (m_htrhIn.isNew()) { m_htrhIn.read();  HumanSynchronizer::Pose3DToHRPPose3D(m_htrh.data,hsp->hp_wld_raw.P[rh]);}
    if (m_htlhIn.isNew()) { m_htlhIn.read();  HumanSynchronizer::Pose3DToHRPPose3D(m_htlh.data,hsp->hp_wld_raw.P[lh]);}
    if (m_htheadIn.isNew()){ m_htheadIn.read(); HumanSynchronizer::Pose3DToHRPPose3D(m_hthead.data,hsp->hp_wld_raw.P[head]);}
    if (m_htzmpIn.isNew()){ m_htzmpIn.read();  HumanSynchronizer::Point3DToVector3(m_htzmp.data,hsp->hp_wld_raw.P[zmp].p); }
//    if (m_actzmpIn.isNew()){m_actzmpIn.read(); }

    if ( is_legged_robot ) {

      bool is_q_interpolator_empty = q_interpolator->isEmpty();
      if (!is_q_interpolator_empty && hsp->isHumanSyncOn()) {
        q_interpolator->get(&q_interpolator_ratio, true);
      }

      if(!hsp->isHumanSyncOn()){//初期姿勢取得＆FK
        m_robot->rootLink()->p = m_robot_rmc->rootLink()->p = hrp::Vector3(m_basePos.data.x, m_basePos.data.y, m_basePos.data.z);
        m_robot->rootLink()->R = m_robot_rmc->rootLink()->R = hrp::rotFromRpy(m_baseRpy.data.r, m_baseRpy.data.p, m_baseRpy.data.y);
        for ( unsigned int i = 0; i < m_robot->numJoints(); i++ ){ m_robot->joint(i)->q = m_robot_rmc->joint(i)->q = m_qRef.data[i]; }
        m_robot->calcForwardKinematics();
        m_robot_rmc->calcForwardKinematics();

        fik->setReferenceJointAngles();
        fik_rmc->setReferenceJointAngles();
      }

      fik->current_tm = fik_rmc->current_tm = m_qRef.tm;
      fik->ikp["rleg"].is_ik_enable = fik_rmc->ikp["rleg"].is_ik_enable = true;
      fik->ikp["lleg"].is_ik_enable = fik_rmc->ikp["lleg"].is_ik_enable = true;
      fik->ikp["rarm"].is_ik_enable = fik_rmc->ikp["rarm"].is_ik_enable = true;
      fik->ikp["larm"].is_ik_enable = fik_rmc->ikp["larm"].is_ik_enable = true;
//      fik->ikp["rarm"].is_ik_enable = fik_rmc->ikp["rarm"].is_ik_enable = false;
//      fik->ikp["larm"].is_ik_enable = fik_rmc->ikp["larm"].is_ik_enable = false;

      if(hsp->WBMSparam.is_doctor){
        processWholeBodyMasterSlave(fik, m_robot);
      }else{
        processWholeBodyMasterSlave_Raw(fik, m_robot);
      }

      /////// Inverse Dynamics /////////
      if(!idsb.is_initialized){
        idsb.setInitState(m_robot, m_dt);
        invdyn_zmp_filters.resize(3);
        for(int i=0;i<3;i++){
          invdyn_zmp_filters[i].setParameterAsBiquad(25, 1/std::sqrt(2), 1.0/m_dt);
//            invdyn_zmp_filters[i].reset(ref_zmp(i));
        }
        idsb2.setInitState(m_robot, m_dt);
        invdyn_zmp_filters2.resize(3);
        for(int i=0;i<3;i++){
          invdyn_zmp_filters2[i].setParameterAsBiquad(25, 1/std::sqrt(2), 1.0/m_dt);
//            invdyn_zmp_filters2[i].reset(ref_zmp(i));
        }
      }
      calcAccelerationsForInverseDynamics(m_robot, idsb);
      hrp::Vector3 ref_zmp_invdyn;
      calcWorldZMPFromInverseDynamics(m_robot, idsb, ref_zmp_invdyn);
      for(int i=0;i<3;i++) ref_zmp_invdyn(i) = invdyn_zmp_filters[i].passFilter(ref_zmp_invdyn(i));
      updateInvDynStateBuffer(idsb);

      if(hsp->isHumanSyncOn() && hsp->WBMSparam.is_doctor){
        processMomentumCompensation(fik_rmc, m_robot_rmc);
      }

//      calcAccelerationsForInverseDynamics(m_robot, idsb2);
//      calcWorldZMPFromInverseDynamics(m_robot, idsb2, ref_zmp_invdyn2);
//      updateInvDynStateBuffer(idsb2);

//        ref_zmp = ref_zmp_invdyn;
//        ref_zmp = ref_zmp_invdyn2;
      ref_zmp = hsp->rp_ref_out.P[zmp].p;
//      hrp::BodyPtr m_robot_for_out = m_robot;
      hrp::BodyPtr m_robot_for_out = m_robot_rmc;

      if (hsp->isHumanSyncOn()) {
        for ( unsigned int i = 0; i < m_qRef.data.length(); i++ ){
          m_qRef.data[i] = q_interpolator_ratio * m_robot_for_out->joint(i)->q  + (1 - q_interpolator_ratio) * m_qRef.data[i];
        }
        // basePos
        m_basePos.data.x = m_robot_for_out->rootLink()->p(0);
        m_basePos.data.y = m_robot_for_out->rootLink()->p(1);
        m_basePos.data.z = m_robot_for_out->rootLink()->p(2);
        m_basePos.tm = m_qRef.tm;
        // baseRpy
        hrp::Vector3 baseRpy = hrp::rpyFromRot(m_robot_for_out->rootLink()->R);
        m_baseRpy.data.r = baseRpy(0);
        m_baseRpy.data.p = baseRpy(1);
        m_baseRpy.data.y = baseRpy(2);
        m_baseRpy.tm = m_qRef.tm;
        // zmp
        rel_ref_zmp = m_robot_for_out->rootLink()->R.transpose() * (ref_zmp - m_robot_for_out->rootLink()->p);
        m_zmp.data.x = rel_ref_zmp(0);
        m_zmp.data.y = rel_ref_zmp(1);
        m_zmp.data.z = rel_ref_zmp(2);
        m_zmp.tm = m_qRef.tm;
      }
      hsp->baselinkpose.p = m_robot->rootLink()->p;
      hsp->baselinkpose.rpy = hrp::rpyFromRot(m_robot->rootLink()->R);
    }
    // write
     m_qOut.write();
    m_basePosOut.write();
    m_baseRpyOut.write();
    m_zmpOut.write();
    m_optionalDataOut.write();

#ifdef USE_DEBUG_PORT
    // dbg plot
     m_htcom.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->hp_plot.P[com],m_htcom_dbg.data);
     m_htcom_dbgOut.write();
     m_htrf.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->hp_plot.P[rf],m_htrf_dbg.data);
     m_htrf_dbgOut.write();
     m_htlf.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->hp_plot.P[lf],m_htlf_dbg.data);
     m_htlf_dbgOut.write();
     m_htrh.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->hp_plot.P[rh],m_htrh_dbg.data);
     m_htrh_dbgOut.write();
     m_htlh.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->hp_plot.P[lh],m_htlh_dbg.data);
     m_htlh_dbgOut.write();
     m_hthead.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->hp_plot.P[head],m_hthead_dbg.data);
     m_hthead_dbgOut.write();
     m_htzmp.tm = m_qRef.tm;
     HumanSynchronizer::Vector3ToPoint3D(hsp->rp_ref_out.P[zmp].p,m_rpzmp_dbg.data);
     m_htzmp_dbgOut.write();
     m_htrfw.tm = m_qRef.tm;
     m_htrfw_dbg.data.length(6);
     HumanSynchronizer::Wrench6ToDoubleSeq(hsp->hp_plot.w[rfw],m_htrfw_dbg.data);
     m_htrfw_dbgOut.write();
     m_htlfw.tm = m_qRef.tm;
     m_htlfw_dbg.data.length(6);
     HumanSynchronizer::Wrench6ToDoubleSeq(hsp->hp_plot.w[lfw],m_htlfw_dbg.data);
     m_htlfw_dbgOut.write();
     m_rpcom_dbg.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->rp_ref_out.P[com],m_rpcom_dbg.data);
     m_rpcom_dbgOut.write();
     m_rprf_dbg.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->rp_ref_out.P[rf],m_rprf_dbg.data);
     m_rprf_dbgOut.write();
     m_rplf_dbg.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->rp_ref_out.P[lf],m_rplf_dbg.data);
     m_rplf_dbgOut.write();
     m_rprh_dbg.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->rp_ref_out.P[rh],m_rprh_dbg.data);
     m_rprh_dbgOut.write();
     m_rplh_dbg.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->rp_ref_out.P[lh],m_rplh_dbg.data);
     m_rplh_dbgOut.write();
     m_rphead_dbg.tm = m_qRef.tm;
     HumanSynchronizer::HRPPose3DToPose3D(hsp->rp_ref_out.P[head],m_rphead_dbg.data);
     m_rphead_dbgOut.write();
     m_rpzmp_dbg.tm = m_qRef.tm;
     HumanSynchronizer::Vector3ToPoint3D(hsp->rp_ref_out.P[zmp].p,m_rpzmp_dbg.data);
     m_rpzmp_dbgOut.write();
     m_rpdcp_dbg.tm = m_qRef.tm;
     HumanSynchronizer::Vector3ToPoint3D(hsp->cp_dec,m_rpdcp_dbg.data);
     m_rpdcp_dbgOut.write();
     m_rpacp_dbg.tm = m_qRef.tm;
     HumanSynchronizer::Vector3ToPoint3D(hsp->cp_acc,m_rpacp_dbg.data);
     m_rpacp_dbgOut.write();
     m_invdyn_dbg.tm = m_qRef.tm;
     m_invdyn_dbg.data.length(6);
     HumanSynchronizer::Wrench6ToDoubleSeq(hsp->invdyn_ft,m_invdyn_dbg.data);
     m_invdyn_dbgOut.write();
#endif
     // gettimeofday(&t_calc_end, NULL);
     //  if(DEBUGP)cout<<"t1:"<<(double)(t_calc_end.tv_sec - t_calc_start.tv_sec) + (t_calc_end.tv_usec - t_calc_start.tv_usec)/1000000.0<<endl;
     //  t_calc_start = t_calc_end;
    loop ++;
    return RTC::RTC_OK;
}

void WholeBodyMasterSlave::processWholeBodyMasterSlave(fikPtr& fik_in, hrp::BodyPtr& robot_in){

  if(hsp->startCountdownForHumanSync){
    std::cerr << "[" << m_profile.instance_name << "] Count Down for HumanSync ["<<hsp->getRemainingCountDown()<<"]\r";
    hsp->updateCountDown();
  }
  if(hsp->isHumanSyncOn()){
    if(hsp->ht_first_call){
      std::cerr << "\n[" << m_profile.instance_name << "] Start HumanSync"<< std::endl;
      hsp->ht_first_call = false;
    }
  }else{
    fik_in->storeCurrentParameters();
    fik_in->setReferenceJointAngles();
    hsp->setCurrentInputAsOffset(hsp->hp_wld_raw);
    hsp->calibInitHumanCOMFromZMP();

    const std::string robot_l_names[4] = {"rleg","lleg","rarm","larm"};
    const int human_l_names[4] = {rf,lf,rh,lh};
    for(int i=0;i<4;i++){
      if(fik_in->ikp.count(robot_l_names[i])){
        hsp->rp_ref_out.P[human_l_names[i]].p_offs = fik_in->ikp[robot_l_names[i]].target_link->p + fik_in->ikp[robot_l_names[i]].target_link->R * fik_in->ikp[robot_l_names[i]].localPos;
        hsp->rp_ref_out.P[human_l_names[i]].rpy_offs = hrp::rpyFromRot(fik_in->ikp[robot_l_names[i]].target_link->R * fik_in->ikp[robot_l_names[i]].localR);
        hsp->rp_ref_out.P[human_l_names[i]].p = hsp->rp_ref_out.P[human_l_names[i]].p_offs;
        hsp->rp_ref_out.P[human_l_names[i]].rpy = hsp->rp_ref_out.P[human_l_names[i]].rpy_offs;
      }
    }
    //    hsp->rp_ref_out.P[com].p_offs = robot_in->calcCM();
    hsp->rp_ref_out.P[com].p_offs(0) = (hsp->rp_ref_out.P[rf].p_offs(0) + hsp->rp_ref_out.P[lf].p_offs(0)) / 2;
    hsp->rp_ref_out.P[com].p_offs(1) = (hsp->rp_ref_out.P[rf].p_offs(1) + hsp->rp_ref_out.P[lf].p_offs(1)) / 2;
    hsp->rp_ref_out.P[com].p_offs(2) = robot_in->calcCM()(2);
    //    hsp->rp_ref_out.P[zmp].p_offs = ref_zmp;
    hsp->pre_cont_rfpos = hsp->rp_ref_out.P[rf].p_offs;
    hsp->pre_cont_lfpos = hsp->rp_ref_out.P[lf].p_offs;
    hsp->baselinkpose.p_offs = robot_in->rootLink()->p;
    hsp->baselinkpose.rpy_offs = hrp::rpyFromRot(robot_in->rootLink()->R);
  }

  hrp::Vector3 rsole_pos = fik_in->ikp["rleg"].target_link->p+ fik_in->ikp["rleg"].target_link->R * fik_in->ikp["rleg"].localPos;
  hrp::Vector3 lsole_pos = fik_in->ikp["lleg"].target_link->p+ fik_in->ikp["lleg"].target_link->R * fik_in->ikp["lleg"].localPos;
  hsp->H_cur = robot_in->calcCM()(2) - std::min((double)rsole_pos(2), (double)lsole_pos(2));

  hsp->update();//////HumanSynchronizerの主要処理
  if(DEBUGP)cout<<"update():"<<hsp->getUpdateTime()<<endl;
  if(DEBUGP)hsp->rp_ref_out.print();

  robot_in->calcForwardKinematics();
  if(hsp->isHumanSyncOn()){
    solveFullbodyIKStrictCOM(fik, m_robot, hsp->rp_ref_out.P[com], hsp->rp_ref_out.P[rf], hsp->rp_ref_out.P[lf], hsp->rp_ref_out.P[rh], hsp->rp_ref_out.P[lh], hsp->rp_ref_out.P[head],"processWholeBodyMasterSlave");
    hsp->rp_ref_out.P[zmp].p(2) = (hsp->rp_ref_out.P[rf].p(2) + hsp->rp_ref_out.P[lf].p(2))/2;//体幹相対ZMP高さ設定
    if(m_optionalData.data.length() < 4*2){
      m_optionalData.data.length(4*2);//これいいのか？
      for(int i=0;i<4*2;i++)m_optionalData.data[i] = 0;
    }
    m_optionalData.data[contact_states_index_map["rleg"]] = m_optionalData.data[contact_states_index_map["rleg"]+4] = hsp->is_rf_contact;
    m_optionalData.data[contact_states_index_map["lleg"]] = m_optionalData.data[contact_states_index_map["lleg"]+4] = hsp->is_lf_contact;
  }
}

void WholeBodyMasterSlave::processWholeBodyMasterSlave_Raw(fikPtr& fik_in, hrp::BodyPtr& robot_in){
  static HumanPose raw_pose;
  static BiquadIIRFilterVec pos_filters[num_pose_tgt], rot_filters[num_pose_tgt];
  static unsigned int callnum;
  if(callnum++ == 0){
    for(int i=0;i<num_pose_tgt;i++){
      pos_filters[i].setParameter(100.0, 1.0/m_dt, BiquadIIRFilterVec::Q_NOOVERSHOOT);
      rot_filters[i].setParameter(100.0, 1.0/m_dt, BiquadIIRFilterVec::Q_NOOVERSHOOT);
    }
  }

  if(hsp->startCountdownForHumanSync){
   std::cerr << "[" << m_profile.instance_name << "] Count Down for RawSync ["<<hsp->getRemainingCountDown()<<"]\r";
   hsp->updateCountDown();
  }
  if(hsp->isHumanSyncOn()){
   if(hsp->ht_first_call){
     std::cerr << "\n[" << m_profile.instance_name << "] Start RawSync"<< std::endl;
     hsp->ht_first_call = false;
   }
  }else{
   //初期状態取得
   fik_in->storeCurrentParameters();
   fik_in->setReferenceJointAngles();
   const std::string robot_l_names[4] = {"rleg","lleg","rarm","larm"};
   const int human_l_names[4] = {rf,lf,rh,lh};
   raw_pose.P[com].p = raw_pose.P[com].p_offs = robot_in->calcCM();
   raw_pose.P[com].rpy = raw_pose.P[com].rpy_offs = hrp::rpyFromRot(robot_in->rootLink()->R);
   pos_filters[com].init(raw_pose.P[com].p);
   rot_filters[com].init(raw_pose.P[com].rpy);
   pos_filters[zmp].init(hrp::Vector3::Zero());
   for(int i=0;i<4;i++){
     if(fik_in->ikp.count(robot_l_names[i])){
       raw_pose.P[human_l_names[i]].p_offs = fik_in->ikp[robot_l_names[i]].target_link->p + fik_in->ikp[robot_l_names[i]].target_link->R * fik_in->ikp[robot_l_names[i]].localPos;
       raw_pose.P[human_l_names[i]].rpy_offs = hrp::rpyFromRot(fik_in->ikp[robot_l_names[i]].target_link->R * fik_in->ikp[robot_l_names[i]].localR);
       raw_pose.P[human_l_names[i]].p = hsp->rp_ref_out.P[human_l_names[i]].p_offs;
       raw_pose.P[human_l_names[i]].rpy = hsp->rp_ref_out.P[human_l_names[i]].rpy_offs;
       pos_filters[human_l_names[i]].init(raw_pose.P[human_l_names[i]].p);
       rot_filters[human_l_names[i]].init(raw_pose.P[human_l_names[i]].rpy);
     }
   }
  }
  robot_in->calcForwardKinematics();
  if(DEBUGP){ fprintf(stderr,"\x1b[31mmaster-mode:\x1b[39m"); raw_pose.print(); }
  if(!hsp->isHumanSyncOn()){
    q_normal_ik.resize(robot_in->numJoints());
    for( int i=0; i<robot_in->numJoints(); i++){ q_normal_ik(i) = robot_in->joint(i)->q; }
  }
  if(hsp->isHumanSyncOn()){
    for(int i=0;i<num_pose_tgt;i++){
      raw_pose.P[i].p = pos_filters[i].passFilter(hsp->hp_wld_raw.P[i].p);
      raw_pose.P[i].rpy = rot_filters[i].passFilter(hsp->hp_wld_raw.P[i].rpy);
    }
    solveFullbodyIKStrictCOM(fik, m_robot, raw_pose.P[com], raw_pose.P[rf], raw_pose.P[lf], raw_pose.P[rh], raw_pose.P[lh], raw_pose.P[head] );
  }
}

void WholeBodyMasterSlave::solveFullbodyIKStrictCOM(fikPtr& fik_in, hrp::BodyPtr& robot_in, const HRPPose3D& com_ref, const HRPPose3D& rf_ref, const HRPPose3D& lf_ref, const HRPPose3D& rh_ref, const HRPPose3D& lh_ref, const HRPPose3D& head_ref, const std::string& debug_prefix){
  int com_ik_loop=0;
  const int COM_IK_MAX_LOOP = 10;
  const double COM_IK_MAX_ERROR = 1e-5;//1e-4だと乱れる
//  robot_in->rootLink()->p = 0.01*com_ref.p + 0.99*robot_in->rootLink()->p;//ベースリンク位置が迷走するのを防ぐ？
  robot_in->rootLink()->R = hrp::rotFromRpy(com_ref.rpy);//move base link at first
  const std::string names[4] = {"rleg","lleg","rarm","larm"};
  const HRPPose3D* refs[4] = {&rf_ref, &lf_ref, &rh_ref, &lh_ref};
  for(int i=0;i<4;i++){
    if(fik_in->ikp.count(names[i])){
      fik_in->ikp[names[i]].target_r0 = hrp::rotFromRpy(refs[i]->rpy);
      fik_in->ikp[names[i]].target_p0 = refs[i]->p;
    }
  }
  fik_in->storeCurrentParameters();
//  fik_in->setReferenceJointAngles();//これ入れると腕ブワーなる
  if(robot_in->link("HEAD_JOINT0") != NULL)robot_in->joint(15)->q = head_ref.rpy(y);
  if(robot_in->link("HEAD_JOINT1") != NULL)robot_in->joint(16)->q = head_ref.rpy(p);
  hrp::Vector3 tmp_com_err = hrp::Vector3::Zero();
  while( 1 ){  //COM 収束ループ
    com_ik_loop++;
    robot_in->rootLink()->p += tmp_com_err;
    robot_in->calcForwardKinematics();
    for ( std::map<std::string, SimpleFullbodyInverseKinematicsSolver::IKparam>::iterator it = fik_in->ikp.begin(); it != fik_in->ikp.end(); it++ ) {
        if (it->second.is_ik_enable) fik_in->solveLimbIK (it->second, it->first, fik_in->ratio_for_vel, false);
    }
    tmp_com_err = com_ref.p - robot_in->calcCM();
    if(tmp_com_err.norm() < COM_IK_MAX_ERROR){ break; }
    if(com_ik_loop >= COM_IK_MAX_LOOP){std::cerr << "COM constraint IK MAX loop [="<<COM_IK_MAX_LOOP<<"] exceeded!! @ "<<debug_prefix<< std::endl; break; };
  }
  if(com_ik_loop != 1)cout<<"com_ik_loop:"<<com_ik_loop<<" @ "<<debug_prefix<<endl;
}

void WholeBodyMasterSlave::processMomentumCompensation(fikPtr& fik_in, hrp::BodyPtr& robot_in){

  hrp::Vector3 P,L;
  m_robot->rootLink()->v = idsb.base_v;
  m_robot->rootLink()->w = idsb.base_w;
  m_robot->calcTotalMomentum(P,L);
  L = L - m_robot->calcCM().cross(P);
  static hrp::Vector3 torso_rot;
  hrp::Vector3 torso_rot_vel;

  m_robot->calcCM();
  m_robot->rootLink()->calcSubMassCM();
  hrp::Matrix33 I_com;

  if(m_robot->link("CHEST_JOINT0")){
    m_robot->link("CHEST_JOINT0")->calcSubMassInertia(I_com);
    torso_rot_vel = - I_com.inverse() * L * hsp->WBMSparam.upper_body_rmc_ratio;
    torso_rot += torso_rot_vel * m_dt;
  }
  if(hsp->isHumanSyncOn()){
    HRPPose3D com_mod = hsp->rp_ref_out.P[com];
    com_mod.rpy += torso_rot;
    solveFullbodyIKStrictCOM(fik_in, robot_in, com_mod, hsp->rp_ref_out.P[rf], hsp->rp_ref_out.P[lf], hsp->rp_ref_out.P[rh], hsp->rp_ref_out.P[lh], hsp->rp_ref_out.P[head],"calcDynamicsFilterCompensation");
  }
}

bool WholeBodyMasterSlave::startCountDownForWholeBodyMasterSlave(const double sec){
  if(sec >= 0.0 && sec <= 30.0){
    std::cerr << "[" << m_profile.instance_name << "] start Synchronization after "<<sec<<" [s]" << std::endl;
    hsp->countdown_sec = sec;
    hsp->startCountdownForHumanSync = true;
    while (!hsp->isHumanSyncOn()){ usleep(1000); }
    return true;
  }else{
    std::cerr << "[" << m_profile.instance_name << "] Count Down Time must be 0 < T < 30 [s]"<< std::endl;
    return false;
  }
}

bool WholeBodyMasterSlave::stopHumanSync(){
	std::cerr << "[" << m_profile.instance_name << "] not implemented..." << std::endl;
	return true;
}

bool WholeBodyMasterSlave::setWholeBodyMasterSlaveParam(const OpenHRP::WholeBodyMasterSlaveService::WholeBodyMasterSlaveParam& i_param){
  std::cerr << "[" << m_profile.instance_name << "] setWholeBodyMasterSlaveParam" << std::endl;
  hsp->WBMSparam.auto_swing_foot_landing_threshold = i_param.auto_swing_foot_landing_threshold;
  hsp->WBMSparam.foot_vertical_vel_limit_coeff = i_param.foot_vertical_vel_limit_coeff;
  hsp->WBMSparam.human_com_height = i_param.human_com_height;
  hsp->WBMSparam.is_doctor = i_param.is_doctor;
  hsp->WBMSparam.set_com_height_fix = i_param.set_com_height_fix;
  hsp->WBMSparam.set_com_height_fix_val = i_param.set_com_height_fix_val;
  hsp->WBMSparam.swing_foot_height_offset = i_param.swing_foot_height_offset;
  hsp->WBMSparam.swing_foot_max_height = i_param.swing_foot_max_height;
  hsp->WBMSparam.upper_body_rmc_ratio = i_param.upper_body_rmc_ratio;
  hsp->WBMSparam.use_rh = hsp->WBMSparam.use_lh = i_param.use_hands;
  hsp->WBMSparam.use_head = i_param.use_head;
  return true;
}

bool WholeBodyMasterSlave::getWholeBodyMasterSlaveParam(OpenHRP::WholeBodyMasterSlaveService::WholeBodyMasterSlaveParam& i_param){
  std::cerr << "[" << m_profile.instance_name << "] getWholeBodyMasterSlaveParam" << std::endl;
  i_param.auto_swing_foot_landing_threshold = hsp->WBMSparam.auto_swing_foot_landing_threshold;
  i_param.foot_vertical_vel_limit_coeff = hsp->WBMSparam.foot_vertical_vel_limit_coeff;
  i_param.human_com_height = hsp->WBMSparam.human_com_height;
  i_param.is_doctor = hsp->WBMSparam.is_doctor;
  i_param.set_com_height_fix = hsp->WBMSparam.set_com_height_fix;
  i_param.set_com_height_fix_val = hsp->WBMSparam.set_com_height_fix_val;
  i_param.swing_foot_height_offset = hsp->WBMSparam.swing_foot_height_offset;
  i_param.swing_foot_max_height = hsp->WBMSparam.swing_foot_max_height;
  i_param.upper_body_rmc_ratio = hsp->WBMSparam.upper_body_rmc_ratio;
  i_param.use_hands = (hsp->WBMSparam.use_rh || hsp->WBMSparam.use_lh);
  i_param.use_head = hsp->WBMSparam.use_head;
  return true;
}

extern "C"{
    void WholeBodyMasterSlaveInit(RTC::Manager* manager) {
        RTC::Properties profile(WholeBodyMasterSlave_spec);
        manager->registerFactory(profile, RTC::Create<WholeBodyMasterSlave>, RTC::Delete<WholeBodyMasterSlave>);
    }
};