#include "booksim.hpp"
#include <vector>
#include <sstream>
#include <ctime>
#include <cassert>
#include "mesh3D.hpp"
#include "random_utils.hpp"
#include "misc_utils.hpp"
#include "roundrobin_arb.hpp"

Mesh3D::Mesh3D( const Configuration &config, const string & name ) :
Network( config, name )
{
  _vchannels = -1;
  _ComputeSize( config );
  _Alloc( );
  _AllocVerChan(config );
  _BuildNet( config );
}

void Mesh3D::_ComputeSize( const Configuration &config )
{
  //in booksim the channel node->router is not part of the channel array
  // it is part of the inject and eject arrays
  _k = config.GetInt( "k" );
  _n = config.GetInt( "n" );
  _s = config.GetInt( "s" );  //jgardea

  gK = _k; gN = _n; gS = _s;   //jgardea
  _size = _k * _n * _s; 
  _vchannels = _k * _n * 2;    //number of vertical channels
  _channels = ( ( ( (_k*(_n-1)) + (_n*(_k-1)) ) * _s )  ) * 2;  // not considerintg vertical channels

  _nodes = _size;
}

void Mesh3D::_AllocVerChan( const Configuration &config ) 
{
  assert( _vchannels != -1 );
  _chan_ver.resize(_vchannels);
  _chan_cred.resize( _channels + _vchannels );
  for ( int cv = 0; cv < _vchannels; cv++)
  {
     ostringstream name;
     name << Name() << "_vchan_" << cv;
     _chan_ver[cv] = new VerticalChannel(this, name.str(), _classes);
     _timed_modules.push_back(_chan_ver[cv]);
     name.str("");
     name << Name() << "_cchan_" << (cv+_channels);
     _chan_cred[cv+_channels] = new CreditChannel(this, name.str());
     _timed_modules.push_back(_chan_cred[cv+_channels]);
  }

  // Alloc vertical channel  arbiters
  string verc_arb_type = config.GetStr( "verc_arbiter" );
  ostringstream verc_arb_name;

  for ( int cv_arb = 0; cv_arb < _vchannels; cv_arb++ )
  {
    verc_arb_name << "verc_arbiter_";
    verc_arb_name << cv_arb; 
    RoundRobinArbiter* arbiter = (RoundRobinArbiter*) Arbiter::NewArbiter( this, verc_arb_name.str(), verc_arb_type, _s);
    _verchan_arbiter.push_back(arbiter);
    verc_arb_name.str("");
  }
}

void Mesh3D::RegisterRoutingFunctions() {
	// routing functions ?
}

void Mesh3D::Evaluate( )
{
  for(deque<TimedModule *>::const_iterator iter = _timed_modules.begin(); iter != _timed_modules.end(); ++iter) {

     (*iter)->Evaluate( );
          
  }

  // =================== Vertical Arbitration ========================= jgardea
  // It is necessary to devide iq3d routers evaluation to pre and post swich allocation 
  // to make sure we have the all the inputs to the vertical arbitraiton

  vector<Router*>::iterator iter;
  IQ3DRouter* r;

  for( iter = _routers.begin(); iter != _routers.end(); iter++ )
  {
    r = (IQ3DRouter *) (*iter);
    r->Evaluate_AfterSA( );
  }

  for( iter = _routers.begin(); iter != _routers.end(); iter++ )
  {
    r = (IQ3DRouter *) (*iter);
      r->ClearVerticalArbiters();
  }
}

void Mesh3D::_BuildNet( const Configuration &config )
{
  int adj_node;
  int adj_input;
  int adj_output;
  int credit_channel;

  ostringstream router_name;

  int ports;
  int x, y, z;
  int stack_size = _n*_k;

  int latency = config.GetInt("channel_latency"); // channel latency 
  RoundRobinArbiter *up_arbiter, *down_arbiter;

  for ( int node = 0; node < _size; ++node ) 
  {
    x = findCoordinate( _x, node );
    y = findCoordinate( _y, node );
    z = findCoordinate( _z, node );
    
    router_name << "router_" << x << y << z;

    if ( (x == _k-1 || x ==  0)  && (y == _n-1 || y == 0) ) ports = 5; 
    else if ( (x > 0 && x < (_k-1)) && (y > 0 && y < (_n-1)) )  ports = 7;   
    else ports = 6;
    
    up_arbiter = _verchan_arbiter[node % stack_size]; // get up vertical channel arbiter  for this router
    down_arbiter = _verchan_arbiter[(node%stack_size) + stack_size];

    _routers[node] = Router::NewRouter( config, this, router_name.str( ), node, ports, ports, up_arbiter, down_arbiter );

    _timed_modules.push_back(_routers[node]);

    router_name.str("");

    // sets all channels in the x and y dimensions

    for ( int p = 0; p < 4; p++ )
    {

      if ( (adj_node = _adjacentNode( node, p )) == NETWORK_EDGE )
        continue;
      
      _routers[node]->updatePortMap( p );

      // Current (N)ode
      // (L)eft node
      // (R)ight node
      //
      //   L<---N<----R
      //   L--->N---->R
      //

       //get channel number from adjacent node conneced through p to this node
      adj_input = _adjacentChannel( adj_node, (p%2 ? p-1 : p+1 )  );

      _routers[node]->AddInputChannel( _chan[adj_input], _chan_cred[adj_input] );
      _chan[adj_input]->SetLatency( latency );
      _chan_cred[adj_input]->SetLatency( latency );
      
      adj_output = _adjacentChannel( node, p);
      _routers[node]->AddOutputChannel( _chan[adj_output], _chan_cred[adj_output] );
      _chan[adj_output]->SetLatency( latency );
      _chan_cred[adj_output]->SetLatency( latency );

    }

    /* ==================== VERTICAL CHANNEL ===================================
     *Each router needs two input ports and two output ports for each vertical channel
     *  vertical channels { 0   , ... , (n*k)-1 } receive data from lower stacks
     *  vertical channels {(n*k), ... , 2(n*k)-1} recieve data from higher stacks
     */

        // add input and output port for the channel receiving from lower stacks ( portnumber = (ports/2)-3 )
    adj_input = node%stack_size; 
    credit_channel = _channels + adj_input;  

    

    _routers[node]->AddInputChannel( _chan_ver[adj_input], _chan_cred[credit_channel], true );
    _routers[node]->AddOutputChannel( _chan_ver[adj_input], _chan_cred[credit_channel], true );
    _routers[node]->updatePortMap( _up );
      
    _chan_ver[adj_input]->SetLatency( latency );
    _chan_cred[credit_channel]->SetLatency( latency );

        // add input and output to the channel receiving from high stacks
    adj_output = (node%stack_size) + stack_size;
    credit_channel = _channels + adj_output;

    _routers[node]->AddOutputChannel( _chan_ver[adj_output], _chan_cred[credit_channel], true );
    _routers[node]->AddInputChannel( _chan_ver[adj_output], _chan_cred[credit_channel], true );
    _routers[node]->updatePortMap( _down );

    _chan_ver[adj_output]->SetLatency( latency );
    _chan_cred[credit_channel]->SetLatency( latency );

        // channel utilization variables for vertical channels
    _chan_ver[adj_input]->setX(x);
    _chan_ver[adj_input]->setY(y);
    _chan_ver[adj_input]->setDir(true);   // going up

    _chan_ver[adj_output]->setX(x);
    _chan_ver[adj_output]->setY(y);
    _chan_ver[adj_output]->setDir(false); // going down
   
    /* ========================================================================= */

        //injection and ejection channel
    _routers[node]->AddInputChannel( _inject[node], _inject_cred[node] );
    _routers[node]->AddOutputChannel( _eject[node], _eject_cred[node] );
    _routers[node]->updatePortMap( _nodeport );

    _inject[node]->SetLatency( latency  );
    _eject[node]->SetLatency( latency );
    
  }
}

int Mesh3D::_adjacentChannel( int node, int dir )
{
  int adjChannel = -1 ;

  int x = findCoordinate( _x, node );
  int y = findCoordinate( _y, node );
  int z = findCoordinate( _z, node );

  int ch_per_k = 4*_k-2;
  int ch_per_n = ( ( (_k-1)*_n ) + ( (_n-1)*_k ) ) * 2 * z;  
  
  int loc_in_n = (ch_per_k * y);
  int offset = loc_in_n + ch_per_n;
  
  switch( dir ) 
  {
    case _left:

      if ((y >=0) && y < (_n-1) )
        adjChannel = (4*(x-1)+1) + offset;
      else if ( y == (_n-1) )
        adjChannel = (2*(x-1)+1) + offset;
      else 
        Error("y value out of bounds of mesh dimensions" );
      break;

    case _right:
     
      if ((y >=0) && y < (_n-1) )
        adjChannel = (4*x) + offset;
      else if ( y == (_n-1) )
        adjChannel = (2*x) + offset;
      else 
        Error("y value out of bounds of mesh dimensions" );
      break;
      
    case _front:
      
      if ((x >=0) && x < (_k-1) )
        adjChannel = (4*x+2) + offset;
      else if ( x == (_k-1) )
        adjChannel = (4*x ) + offset;
      else 
        Error("x value out of bounds of mesh dimensions" );
      break;
      
    case _back:

      if ((x >=0) && x < (_k-1) )
        adjChannel = (4*x+3) + ( ch_per_k * (y-1) ) + ch_per_n;
      else if ( x == (_k-1) )
        adjChannel = (4*x+1) + ( ch_per_k * (y-1) ) + ch_per_n;
      else 
        Error("x value out of bounds of mesh dimensions" );
      break;   
    
    default:
      Error( "The port does not exist (Mesh3D::adjacentChannel)" );

  }
  return adjChannel;

}

int Mesh3D::_adjacentNode( int node, int dir )
{
  int adjNode = NETWORK_EDGE;

  switch( dir ) 
  {
    case _left:

      if ( !(findCoordinate(_x, node) == 0) )
        return node-1;
      break;

    case _right:

      if ( !(findCoordinate( _x, node ) == (_k-1)) )
        return node+1;
      break;

    case _front:
      
      if ( !(findCoordinate( _y, node ) == (_n-1)) )
        return node+_k;
      break;

    case _back:

       if ( !(findCoordinate( _y, node ) == 0) )
        return node-_k;
      break;   
    default:
      Error( "The port does not exist (Mesh3D::adjacentNode)" );

  }
  return adjNode;
}

int Mesh3D::findCoordinate( int cord, int node )
{
  int value = 0;
  int stack_size = _k*_n;

  switch( cord )
  {
    case _x:
      value = node%_k;
      break;
    case _y:
      value = ( node - ( (node/stack_size) * stack_size) )/_k;
      break;
    case _z:
      value = (node / stack_size);
      break;
    default:
      Error("Coordinate does not exist");
  }
  return value;
}

int Mesh3D::getN( ) const
{
  return _n;
}

int Mesh3D::getK( ) const
{
  return _k;
}

int Mesh3D::getS( ) const
{
  return _s;
}

double Mesh3D::Capacity( ) const
{
  return (double)_k / 8;
}

Mesh3D::~Mesh3D( )
{
  for ( int cv = 0; cv < _vchannels ; cv++ ) 
  {
    if ( _chan_ver[cv] ) delete _chan_ver[cv]; // TODO: delete its credit channels
  }
  for ( int a = 0; a < _vchannels; a++ ) 
  {
    if ( _verchan_arbiter[a] ) delete _verchan_arbiter[a];
  }
}                                  
