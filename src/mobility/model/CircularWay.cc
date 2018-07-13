/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015,2016 CONELAB UOU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Md Mehedi Hasan <hasan3345@gmail.com>
 *
 * Modified by Carlos Herranz <carhercl@iteam.upv.es>
 */

#include "CircularWay.h"
#include "ns3/enum.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CircularWayMobility");

NS_OBJECT_ENSURE_REGISTERED (CircularWay);


TypeId
CircularWay::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CircularWay")
    .SetParent<MobilityModel> ()
    .SetGroupName ("Mobility")
    .AddConstructor<CircularWay> ()
    .AddAttribute ("Radius",
                   "A double value used to pick the radius of the way (m).",
                   DoubleValue (10.0),
                   MakeDoubleAccessor (&CircularWay::m_radius),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Speed",
				   "A double value used to pick the speed (m/s).",
				   DoubleValue (1.0),
				   MakeDoubleAccessor (&CircularWay::m_speed),
				   MakeDoubleChecker<double> ())
    .AddAttribute ("Theta",
				   "A double value used to pick the theta.",
				   DoubleValue (0.0),
				   MakeDoubleAccessor (&CircularWay::m_theta),
				   MakeDoubleChecker<double> ());
  return tid;
}

void
CircularWay::DoInitialize (void)
{
  DoInitializePrivate ();
  MobilityModel::DoInitialize ();
}

void
CircularWay::DoInitializePrivate (void)
{
  m_helper.Update ();
  double pi = M_PI;
  double speed = m_speed;
  double radius = m_radius;
  double omega = speed/radius;
  double theta = GetTheta();

  if((theta + omega)< (2*pi)){
	  theta += omega;
	  SetTheta(theta);
  }
  else{
	  theta = (theta + omega) - (2*pi);
	  SetTheta(theta);
  }

  Vector vector ((radius*std::cos (theta))*omega,
		  	  	 (radius*std::sin (theta))*omega,
                 0.0);
  m_helper.SetVelocity (vector);
  m_helper.Unpause ();
  Time delayLeft = Seconds (1);
  DoWalk (delayLeft);
}

void
CircularWay::DoWalk (Time delayLeft)
{


  Vector position = m_helper.GetCurrentPosition ();
  Vector speed = m_helper.GetVelocity ();
  Vector nextPosition = position;
  nextPosition.x += speed.x * delayLeft.GetSeconds ();
  nextPosition.y += speed.y * delayLeft.GetSeconds ();
  m_event.Cancel ();
  m_event = Simulator::Schedule (delayLeft, &CircularWay::DoInitializePrivate, this);
  NotifyCourseChange ();
}

void
CircularWay::DoDispose (void)
{
  // chain up
  MobilityModel::DoDispose ();
}
Vector
CircularWay::DoGetPosition (void) const
{
  m_helper.Update();
  return m_helper.GetCurrentPosition ();
}
void
CircularWay::DoSetPosition (const Vector &position)
{
  m_helper.SetPosition (position);
  Simulator::Remove (m_event);
  m_event = Simulator::ScheduleNow (&CircularWay::DoInitializePrivate, this);
}
Vector
CircularWay::DoGetVelocity (void) const
{
  return m_helper.GetVelocity ();
}

void
CircularWay::SetTheta (double &theta)
{
	m_theta = theta;
}

double
CircularWay::GetTheta (void) const
{
	return m_theta;
}

} // namespace ns3
