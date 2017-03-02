// ----i------------------------------------------------------------------
//
//  File Name: vertical_channel.cpp
//  Author: Jesus Gardea
//
// ----------------------------------------------------------------------

#include "vertical_channel.hpp"

#include <iostream>
#include <iomanip>

#include "router.hpp"
#include "globals.hpp"
#include "flitchannel.hpp"

VerticalChannel::VerticalChannel(Module * parent, string const & name, int classes)
: FlitChannel(parent, name, classes), _idle(0)  
{
  _active.resize(classes, 0);
  _routerSourcePort = -1;
  _routerSinkPort = -1;
}

void VerticalChannel::SetVerticalSource(Router const * const router, int port) {

  if ( _routerSourcePort < 0) _routerSourcePort = port;
  _routerSources.push_back(router);
}

void VerticalChannel::SetVerticalSink(Router const * const router, int port) 
{
  if ( _routerSinkPort < 0 ) _routerSinkPort = port;
  _routerSinks.push_back(router);
}

Router const* const VerticalChannel::GetVerticalSource( int id ) const 
{
  int i;
  int size = _routerSources.size();

  for ( i = 0; i < size; i++ )
    if ( _routerSources[i]->GetID( ) == id ) break;

  if ( i == size ) Error("Router not a source");
  return _routerSources[i];
}

Router const * const VerticalChannel::GetVerticalSink(int id) const
{
    int i;
    int size = _routerSinks.size();
    int layer_size = gK * gN;

    for ( i = 0; i < size; i++ )
      if ( (_routerSinks[i]->GetID( )/layer_size) == (id/layer_size) ) break;

    if ( i == size ) Error("Router not a sink.");
    return _routerSinks[i];
}
