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
 */

#ifndef CIRCULARWAY_H_
#define CIRCULARWAY_H_

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/rectangle.h"
#include "ns3/random-variable-stream.h"
#include "mobility-model.h"
#include "constant-velocity-helper.h"

namespace ns3 {


/**
 * \ingroup mobility
 * \brief CircularWay.
 *
 * Each instance moves circularly with a given speed.
 */
class CircularWay : public MobilityModel
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

private:
  /**
   * Walk according to the path that creates a circle and with constant velocity.
   */
  void DoWalk (Time timeLeft);
  /**
   * Perform initialization of the object before MobilityModel::DoInitialize ()
   */
  void DoInitializePrivate (void);
  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;
  virtual void SetTheta (double &theta);
  virtual double GetTheta (void) const;
  ConstantVelocityHelper m_helper; //!< helper for this object
  EventId m_event; //!< stored event ID
  double m_speed;
  double m_radius;
  double m_theta;
};



} // namespace ns3

#endif /* CIRCULARWAY_H_ */
