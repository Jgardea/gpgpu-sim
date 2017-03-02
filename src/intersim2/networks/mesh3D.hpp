#ifndef _MESH3D_HPP_
#define _MESH3D_HPP_

#define NETWORK_EDGE -1

#include "network.hpp"
#include "vertical_channel.hpp"
#include "roundrobin_arb.hpp"
#include "flitchannel.hpp"
#include "iq3d_router.hpp"

enum  { _x, _y, _z}; // coordinates of a router

class Mesh3D : public Network {
protected:

  int _k; 
  int _n;
  int _s; 
  int _vchannels;

  vector<VerticalChannel *> _chan_ver;
  vector<RoundRobinArbiter *> _verchan_arbiter;

  virtual void Evaluate( );
  
  void _ComputeSize( const Configuration &config );
  void _AllocVerChan( const Configuration &config );
  void _BuildNet( const Configuration &config );

  int _LeftChannel( int node, int dim );
  int _RightChannel( int node, int dim );

  int _LeftNode( int node, int dim );
  int _RightNode( int node, int dim );
  
  int _adjacentNode( int node, int dir );
  int _adjacentChannel( int node, int dir );

  //vector<int> _adjacentVerticalNodes( int node, int dir); // TODO: needed ? //jgardea

public:
  
  Mesh3D( const Configuration &config, const string & name );
  ~Mesh3D();

  static void RegisterRoutingFunctions();
  
  virtual double Capacity( ) const;

  int getN() const;
  int getK() const;
  int getS() const;
  int findCoordinate( int cord, int node );

  inline int VerticalChannels() const { return _vchannels; }
  inline vector<VerticalChannel *> GetVerticalChannels( ) {
    return _chan_ver;
  }
};

#endif
