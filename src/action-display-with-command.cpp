// Copyright (C) 2010 by Claire Dune, Thomas Moulard, CNRS.
//
// This file is part of the LLVC.
//
// This software is provided "as is" without warranty of any kind,
// either expressed or implied, including but not limited to the
// implied warranties of fitness for a particular purpose.
//
// See the COPYING file for more information.

#include <fstream>

#include <boost/format.hpp>

#include <visp/vpImageIo.h>

/// this include is to have the function getInitFileFromModelName
#include "llvc/action-display-mbt.h"
#include "llvc/action-display-with-command.h"
#include "llvc/action-tracking-with-command.h"
#include "llvc/tools/indent.hh"

// CMAKE_INSTALL_PREFIX is used to know where
// to log debug information.
#ifndef CMAKE_INSTALL_PREFIX
# error "CMAKE_INSTALL_PREFIX should be defined."
#endif //! CMAKE_INSTALL_PREFIX

namespace trackingClient
{
  /// \brief Installation prefix.
  static const std::string prefix = CMAKE_INSTALL_PREFIX;
  /// \brief Logging directory.
  static const std::string loggingDir = prefix + "/var/log";

  ActionDisplayWithCommand::ActionDisplayWithCommand
  (boost::shared_ptr<ActionGrab> gc,
   const std::string& modelName,
   const std::string& configurationName,
   unsigned desPoseNb,
   vpColor color,
   bool logData)
    : ActionDisplay(gc),
      m_trackingClient(),
      m_tracker(),
      m_initialPose(),
      m_desPoseNb(desPoseNb),
      m_color(color),
      m_logData(logData),
      m_modelName(modelName),
      m_configurationName(configurationName)
  {
    m_actionGrabClient->ExecuteAction();
    m_image = m_actionGrabClient->image();
    vpDisplay::display(m_image);
    vpDisplay::flush(m_image);

    // set the client tracker parameters
    // warning : if the parameters of the camera are not set,
    // the computation of the pose is false.
    // so we have to set the tracker parameters
    vpCameraParameters cam = m_actionGrabClient->camera();
    m_tracker.setCameraParameters(cam);
    m_tracker.loadModel(getModelFileFromModelName(modelName).c_str());
    

    homogeneousMatrix_t desiredPoseList;
    
   
    // Initilise the desired pose
    std::cout << "You have to initialise " 
	      << desPoseNb 
	      << "desired positions" 
	      << std::endl;
    for(unsinged i=0; i<desPoseNb;++i)
      {
	std::cout << " Please, move the robot to the desired position number "
		  << i
		  << std::endl
		  << " Click on the image when ready." 
		  << std::endl;
	waitForUserClick();
        clickToInitDesiredPose(desiredPoseList);
      }
    // Initialise the initial pose
    std::cout << " USER >> Please move the robot to the Initial position." 
	      << std::endl;
    std::cout << " Click on the image when ready." 
	      << std::endl;
    waitForUserClick();
    clickToInitPose(m_initialPose);

    // FIXME : specific to mbt. shoulb be changed to be more generix 
    boost::shared_ptr<ActionTracking> tracker = 
      boost::shared_ptr<ActionTracking>
      (new ActionTrackingMbt
       (m_initialPose,
    	m_actionGrabClient,
    	m_modelName,
    	m_configurationName));
    
    m_trackingClient = 
      boost::shared_ptr<ActionTrackingWithCommand>
      (new ActionTrackingWithCommand
       (tracker,
	desiredPoseList	) );
       

    
    if (m_logData)
      m_trackingClient->setTrackingParameters("DATA", "ON");



    ODEBUG("Out of the function :ActionDisplayWithCommand::ActionDisplayWithCommand ");
  }

  ActionDisplayWithCommand::~ActionDisplayWithCommand()
  {
  }
  


  /// wait for a user click
  void  
  ActionDisplayWithCommand::waitForUserClick()
  {
    bool userHasClicked = false; 
    while (!userHasClicked)
      {
	m_actionGrabClient->ExecuteAction();
	m_image = m_actionGrabClient->image();
	vpDisplay::display(m_image);
	vpDisplay::flush(m_image);
	userHasClicked = vpDisplay::getClick(m_image,false);
      }
    std::string textYouClicked = "Click Ok ! continue ... ";
    vpDisplay::displayCharString(m_image, 
				 150, 150, 
				 textYouClicked.c_str(),
				 vpColor::red);
    vpDisplay::flush(m_image);
    sleep(0.5);
    vpDisplay::display(m_image);
    vpDisplay::flush(m_image);
     
     
  }


  bool 
  ActionDisplayWithCommand::Initialize()
  {
    m_trackingClient->Initialize();
    // FIXME : always true
    return ActionDisplay::Initialize();
  }
    

  void
  ActionDisplayWithCommand::clickToInitDesiredPose
  (homogeneousMatrix_t& desiredPoseList)
  {
    vpHomogeneousMatrix desiredPose;
    clickToInitPose(desiredPose);
    desiredPoseList.push_back(desiredPose);
  
    ODEBUG3(" Desired position saved: " << std ::endl);
    for (unsigned i = 0 ; i < desiredPoseList.size();++i) 
      ODEBUG3(  i  << "---\t"
		<< desiredPoseList[i] 
		<< std::endl);
      
  }

  bool 
  ActionDisplayWithCommand::clickToInitPose(vpHomogeneousMatrix & cMo)
  {
    m_tracker.initClick(m_image, 
			getInitFileFromModelName(m_modelName).c_str());
    m_tracker.getPose(cMo);
    return true;
  }


  bool 
  ActionDisplayWithCommand::ExecuteAction()
  {

    m_trackingClient->ExecuteAction();
    m_image = m_trackingClient->image();

    //FIXME: send patch to enhance ViSP constness.
    // I.e. these copies would not be required if the constness
    // was handled correctly by vpMbtTracker.
    vpCameraParameters cam = m_actionGrabClient->camera();
    vpHomogeneousMatrix cMo = m_trackingClient->pose();

    //FIXME: we do *not* grab using the grabbing
    //client anymore as ActionTrackingMbt::getBufferData
    //is giving us a synchronized image.
    //if (!ActionDisplay::ExecuteAction())
    // return false;
    vpDisplay::display(m_image);
    m_tracker.display (m_image, cMo, cam, m_color);
    vpDisplay::flush(m_image);

    if (m_logData)
      logData();

    return true;
  }

  void 
  ActionDisplayWithCommand::CleanUp()
  {
    m_trackingClient->CleanUp();
    ActionDisplay::CleanUp();
  }

  std::ostream&
  ActionDisplayWithCommand::print (std::ostream& stream) const
  {
    ActionDisplay::print (stream);
    stream << iendl
	   << "ActionDisplayWithCommand:" << incindent << iendl
	   << "tracking nb des pose: " << m_desPoseNb << iendl
	   << "tracking color: " << m_color << iendl
	   << "tracking save log: " << m_logData << iendl
	   << "tracking client:" << incindent << iendl;

    if (m_trackingClient)
      stream << *m_trackingClient;
    else
      stream << "no tracking client";

    stream << decindent << decindent;
    return stream;
  }

  namespace
  {
    std::string makeLogFilename(const std::string& filename)
    {
      std::string result(loggingDir);
      result += "/";
      result += filename;
      return result;
    }
  } // end of namespace gnuplot.

  void
  ActionDisplayWithCommand::logData() const
  {
    static std::ofstream file(makeLogFilename("llvc-mbt.log").c_str());
    static unsigned index = 0;

    if (!file)
      throw "failed to log data";

    if (!index)
      {
	file
	  << "# index (1) | timestamp.first (2) |  timestamp.second (3) | "
	  << " translation (4-6) | rx, ry, rz (7-9)" << std::endl;
      }

    const ActionTrackingMbt::image_t& image = m_trackingClient->image();
    const vpHomogeneousMatrix& cMo = m_trackingClient->pose();
    const ActionTrackingMbt::timestamp_t& timestamp = m_trackingClient->timestamp();

    vpThetaUVector thetaUVector(cMo);
    vpRxyzVector rxyzVector(thetaUVector);

    file
      << index << " "
      << timestamp.first << " "
      << timestamp.second << " "
      << cMo[0][3] << " "
      << cMo[1][3] << " "
      << cMo[2][3] << " "
      << rxyzVector[0] << " "
      << rxyzVector[1] << " "
      << rxyzVector[2] << std::endl;
    ++index;

    // ViSP is using overlays.
    // We have to render the overlay *on* the image
    // before storing it on the disk.
    vpImage<vpRGBa> colorImage;
    vpDisplay::getImage(m_image, colorImage);

    boost::format fmtColor("Ires-%04d.ppm");
    fmtColor % index;

    boost::format fmt("I-%04d.pgm");
    fmt % index;

    vpImageIo::writePPM(colorImage, makeLogFilename(fmtColor.str()));
    vpImageIo::writePGM(image, makeLogFilename(fmt.str()));
  }
} // end of namespace trackingClient