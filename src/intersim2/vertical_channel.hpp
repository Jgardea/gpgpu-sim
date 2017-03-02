
// ----------------------------------------------------------------------
//
//  File Name: vertical_channel.hpp
//
//  The vertical channel models a bus style  channel with a single-cycle 
//   transmission delay that connects routers along the z dimension.
// ----------------------------------------------------------------------

#ifndef VERTICALCHANNEL_HPP
#define VERTICALCHANNEL_HPP

// ----------------------------------------------------------------------
//  $Author: jgardea $
//  $Date: 2016/1/2016 $
//  $Id$
// ----------------------------------------------------------------------

#include <utility>
#include <vector>

#include "flitchannel.hpp"

using namespace std;

class Router ;

class VerticalChannel : public FlitChannel {
public:
  VerticalChannel(Module * parent, string const & name, int classes);

  void SetVerticalSource(Router const * const router, int port) ;

  Router const * const GetVerticalSource( int id ) const;
    
  void SetVerticalSink(Router const * const router, int port) ;

  Router const * const GetVerticalSink(int id) const; 

  inline vector<Router const*> Sources( )
  {
    return _routerSources; 
  }

  // channel utilization helper funcitons

  inline short getX( ) { return _x; }

  inline short getY( ) { return _y; }

  inline bool getDir( ) { return _dir; }

  inline void setX( short x ) { _x = x; }

  inline void setY ( short y ) { _y = y; }

  inline void setDir( bool dir) { _dir = dir; }

private:
  
  ///////////////////////////////////////
  //  
  // Vertical channels have sources and sinks equal
  // to # of stacks 
  //
  // Vertical channels only work with symmetric routers,
  // meaning same number of inputs and outputs
  //
  // all inputs and outputs have the same port 
  
  vector<Router const *> _routerSources;
  vector<Router const *> _routerSinks;
 
  // channel utilization helper class members

  short _x;
  short _y;
  bool _dir;

  vector<int> _active;
  int _idle;
};

#endif
