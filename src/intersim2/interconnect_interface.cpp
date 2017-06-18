// Copyright (c) 2009-2013, Tor M. Aamodt, Dongdong Li, Ali Bakhoda
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>
#include <algorithm>

#include "interconnect_interface.hpp"
#include "routefunc.hpp"
#include "globals.hpp"
#include "trafficmanager.hpp"
#include "mem_fetch.h"
#include "power_module.hpp"
#include "flit.hpp"
#include "gputrafficmanager.hpp"
#include "booksim.hpp"
#include "intersim_config.hpp"
#include "network.hpp"
#include "random_utils.hpp"     // jgardea

enum NetSelection { _pckt_type, _round_robin, _random, _pckt_size }; // subnetwork selection jgardea

InterconnectInterface* InterconnectInterface::New(const char* const config_file)
{
  if (! config_file ) {
    cout << "Interconnect Requires a configfile" << endl;
    exit (-1);
  }
  InterconnectInterface* icnt_interface = new InterconnectInterface();
  icnt_interface->_icnt_config = new IntersimConfig();
  icnt_interface->_icnt_config->ParseFile(config_file);
  return icnt_interface;
}

InterconnectInterface::InterconnectInterface()
{

}

InterconnectInterface::~InterconnectInterface() 
{
  for (int i=0; i<_subnets; ++i) {
    delete _net[i];
  }
  delete dsent_module;
  dsent_module = NULL;
  delete _traffic_manager;
  _traffic_manager = NULL;
  delete _icnt_config;
}

void InterconnectInterface::CreateInterconnect(unsigned n_shader, unsigned n_mem)
{
  _n_shader = n_shader;
  _n_mem = n_mem;

  InitializeRoutingMap(*_icnt_config);

  gPrintActivity = (_icnt_config->GetInt("print_activity") > 0);
  gTrace = (_icnt_config->GetInt("viewer_trace") > 0);

  string watch_out_file = _icnt_config->GetStr( "watch_out" );
  if(watch_out_file == "") {
    gWatchOut = NULL;
  } else if(watch_out_file == "-") {
    gWatchOut = &cout;
  } else {
    gWatchOut = new ofstream(watch_out_file.c_str());
  }

  _subnets = _icnt_config->GetInt("subnets");
  assert(_subnets);

  /*To include a new network, must register the network here
   *add an else if statement with the name of the network
   */
  _net.resize(_subnets);
  for (int i = 0; i < _subnets; ++i) {
    ostringstream name;
    name << "network_" << i;
    _net[i] = Network::New( *_icnt_config, name.str() );
  }

  assert(_icnt_config->GetStr("sim_type") == "gpgpusim");  
  _traffic_manager = static_cast<GPUTrafficManager*>(TrafficManager::New( *_icnt_config, _net )) ;
 
  _flit_size = _icnt_config->GetInt( "flit_size" );
  _asym_flit_size = _icnt_config->GetInt( "asym_flit_size" );
  // Config for interface buffers
  if (_icnt_config->GetInt("ejection_buffer_size")) {
    _ejection_buffer_capacity = _icnt_config->GetInt( "ejection_buffer_size" ) ;
  } else {
    _ejection_buffer_capacity = _icnt_config->GetInt( "vc_buf_size" );
  }

  _boundary_buffer_capacity = _icnt_config->GetInt( "boundary_buffer_size" ) ;
  assert(_boundary_buffer_capacity);
  if (_icnt_config->GetInt("input_buffer_size")) {
    _input_buffer_capacity = _icnt_config->GetInt("input_buffer_size");
  } else {
    _input_buffer_capacity = 16;
  }
  _vcs = _icnt_config->GetInt("num_vcs");
    
  _CreateBuffer();

  _CreateNodeMap(_n_shader, _n_mem, _traffic_manager->_nodes, _icnt_config->GetInt("use_map"));
}

void InterconnectInterface::Init()
{
  _traffic_manager->Init();
}

//TODO: move to _IssuePacket
//TODO: create a Inject and wrap _IssuePacket and _GeneratePacket
void InterconnectInterface::Push(unsigned input_deviceID, unsigned output_deviceID, void *data, unsigned int size)
{
  // it should have free buffer
  assert(HasBuffer(input_deviceID, size));

  int output_icntID = _node_map[output_deviceID];
  int input_icntID = _node_map[input_deviceID];

  // Flits will be measured in bits for DSENT power model so I multiplied the size of the incoming data by 8
  int size_n_bits  = size * 8; // jgardea
  unsigned int n_flits = (size_n_bits) / _flit_size + ((size_n_bits % _flit_size)? 1:0);
 
  if( gNetSelec == _pckt_size ) n_flits = (size_n_bits) / _asym_flit_size + ((size_n_bits % _asym_flit_size)?1:0);

  //TODO: Remove mem_fetch to reduce dependency
  Flit::FlitType packet_type;
  mem_fetch* mf = static_cast<mem_fetch*>(data);

  switch (mf->get_type()) {
    case READ_REQUEST:  packet_type = Flit::READ_REQUEST   ;break;
    case WRITE_REQUEST: packet_type = Flit::WRITE_REQUEST  ;break;
    case READ_REPLY:    packet_type = Flit::READ_REPLY     ;break;
    case WRITE_ACK:     packet_type = Flit::WRITE_REPLY    ;break;
    default: assert (0);
  }

  int subnet;
  for ( int net = 0; net < _subnets; net++ )
  {
	  subnet = NetworkSelection(packet_type, input_deviceID);
	  if (NetworkHasBuffer(subnet, input_icntID, size, n_flits)) break;
	  subnet = -1;
  }
 
  //it should have subnet >= 0
  assert(subnet >= 0);

  if (0) 
  	cout << "Input node: " << input_deviceID << " Output Node: " << output_deviceID << " Size: " << size_n_bits 
  	     << " Flit size: " << ((gNetSelec == _pckt_size) ? _asym_flit_size : _flit_size) << " Num-Flits: " << n_flits << " Subnet: " << subnet << " Type: " 
		 << packet_type << endl;

  //TODO: _include_queuing ?
  _traffic_manager->_GeneratePacket( input_icntID, -1, 0 /*class*/, _traffic_manager->_time, subnet, n_flits, packet_type, data, output_icntID);

#if DOUB
  cout <<"Traffic[" << subnet << "] (mapped) sending form "<< input_icntID << " to " << output_icntID << endl;
#endif
//  }
}

void* InterconnectInterface::Pop(unsigned deviceID)
{
  int icntID = _node_map[deviceID];
#if DEBUG
  cout<<"Call interconnect POP  " << output<<endl;
#endif

  void* data = NULL;

  // 0-_n_shader-1 indicates reply(network 1), otherwise request(network 0)
 // int subnet = 0;
 // if (deviceID < _n_shader)
  //  subnet = 1;
 
  int turn; 
  int subnet = _round_robin_subnet_turn[icntID];                       // jgardea // fair check of all the networks

  for ( int sn = 0; sn < _subnets; sn++)                                // jgardea
  {
    
    turn = _round_robin_turn[subnet][icntID];

    for (int vc=0;(vc<_vcs) && (data==NULL);vc++) {                  // original code
     	
      if (_boundary_buffer[subnet][icntID][turn].HasPacket()) {
        data = _boundary_buffer[subnet][icntID][turn].PopPacket();
      }
      
      turn++;
      if (turn == _vcs) turn = 0;
    }
    
    subnet++;                                                         // jgardea
    if ( subnet == _subnets ) subnet = 0;                             // jgardea

    if (data) {                                                       // original code
      _round_robin_turn[subnet][icntID] = turn;
      _round_robin_subnet_turn[icntID] = subnet;                      // jgardea
      break;                                                          // jgardea  
    }
  }
  
  return data;

}

void InterconnectInterface::Advance()
{
  _traffic_manager->_Step();
}

bool InterconnectInterface::Busy() const
{
  // 0 is for class
  bool busy = !_traffic_manager->_total_in_flight_flits[0].empty();
  if (!busy) {
    for (int s = 0; s < _subnets; ++s) {
      for (unsigned n = 0; n < _n_shader+_n_mem; ++n) {
        //FIXME: if this cannot make sure _partial_packets is empty
        assert(_traffic_manager->_input_queue[s][n][0].empty());
      }
    }
  }
  else
    return true;
  for (int s = 0; s < _subnets; ++s) {
    for (unsigned n=0; n < (_n_shader+_n_mem); ++n) {
      for (int vc=0; vc<_vcs; ++vc) {
        if (_boundary_buffer[s][n][vc].HasPacket() ) {
          return true;
        }
      }
    }
  }
  return false;
}


// Subnet Selection  // jgardea
 // Added a network selction function based on the configurations I am using in Booksim

int InterconnectInterface::NetworkSelection( int pckt_info, unsigned icntID  )
{
	int subnet = 0;
	switch (gNetSelec)  // jgardea
	{
		case _pckt_type:   // default  
		  if (icntID < _n_shader ) subnet = 0;
		  else subnet = 1;
		  break;
		case _round_robin:		// default for 4 symmetric subnetworks
		  subnet = gSubnet; 
		  gSubnet++;
		  if ( gSubnet == _subnets ) gSubnet = 0;
		  break;
		case _random: 			// Randonm subnet selection
		  subnet = RandomInt(_subnets-1);
		  break;
		case _pckt_size:			// For asymmetric subnets based on packet size
		  switch (pckt_info)
		  {
			case READ_REQUEST:
			case WRITE_ACK:
			  subnet= 0;
			  break;
			case READ_REPLY:
			case WRITE_REQUEST:
			  subnet = 1;
			  break;
			default: assert(0);
		  }
		  break;
		 default: assert(0);
	}
	return subnet;
}

// For multiple networks it is necessary to check which network is avialable
bool InterconnectInterface::NetworkHasBuffer(int subnet, unsigned icntID, unsigned int size, unsigned int n_flits ) const
{
	return  ((_traffic_manager->_input_queue[subnet][icntID][0].size() + n_flits) <= _input_buffer_capacity);
}

bool InterconnectInterface::HasBuffer(unsigned deviceID, unsigned int size) const
{
  // flits are measured in bits 									// jgardea
  // Also added a way to check all subnets 

  bool has_buffer = false;
  unsigned int size_n_bits = size * 8;
  unsigned int n_flits = size_n_bits / _flit_size + ((size_n_bits % _flit_size)? 1:0);

  int icntID = _node_map.find(deviceID)->second;

  int subnet = 0;
  switch (gNetSelec)  // jgardea
  {
  	case _pckt_type:   // default 
	  subnet = (deviceID > _n_shader);
	  has_buffer = _traffic_manager->_input_queue[subnet][icntID][0].size() +n_flits <= _input_buffer_capacity;
  	  break;
  	case _round_robin:		// default for 4 symmetric subnetworks
	case _random:
	  for ( int net = 0; net < _subnets; net++)
	  {
	    has_buffer = _traffic_manager->_input_queue[net][icntID][0].size() +n_flits <= _input_buffer_capacity;
		if (has_buffer) break;
	  }
	  break;
  	case _pckt_size:			// For asymmetric subnets based on packet size
	  if ( size_n_bits <= _flit_size ) subnet= 0;
	  else 
      { // If packets is going through the wide subnet then the number of flit is different
        subnet = 1;
        n_flits = size_n_bits / _asym_flit_size + ((size_n_bits % _asym_flit_size)? 1:0);
      }
	  has_buffer = _traffic_manager->_input_queue[subnet][icntID][0].size() +n_flits <= _input_buffer_capacity;
	  break;
  	 default: assert(0);
  }

  //has_buffer = _traffic_manager->_input_queue[0][icntID][0].size() +n_flits <= _input_buffer_capacity;

  //if ((_subnets>1) && deviceID >= _n_shader) // deviceID is memory node
  //  has_buffer = _traffic_manager->_input_queue[1][icntID][0].size() +n_flits <= _input_buffer_capacity;

  return has_buffer;
}

void InterconnectInterface::DisplayStats() const
{
  _traffic_manager->UpdateStats();
  //_traffic_manager->DisplayStats();
}

unsigned InterconnectInterface::GetFlitSize() const
{
  return _flit_size;
}

void InterconnectInterface::DisplayOverallStats() //const
{
  // hack: booksim2 use _drain_time and calculate delta time based on it, but we don't, change this if you have a better idea
  _traffic_manager->_drain_time = _traffic_manager->_time;
  // hack: also _total_sims equals to number of kernel calls
  _traffic_manager->_total_sims += 1;

  _traffic_manager->_UpdateOverallStats();
  _traffic_manager->DisplayOverallStats();
  _traffic_manager->_ChannelUtilizationStats(); // jgardea
  if(_traffic_manager->_print_csv_results) {
    _traffic_manager->DisplayOverallStatsCSV();
  }
  
  //jgardea DSENT power module
  if ( _icnt_config->GetInt("sim_power") > 0 )
  {
	  if ( !dsent_module ) dsent_module = new Power_Module(_net, *_icnt_config);
      //Power_Module pnet(_net, *_icnt_config);
      dsent_module->dsent();
  }
}

void InterconnectInterface::DisplayState(FILE *fp) const
{
  fprintf(fp, "GPGPU-Sim uArch: ICNT:Display State: Under implementation\n");
//  fprintf(fp,"GPGPU-Sim uArch: interconnect busy state\n");

//  for (unsigned i=0; i<net_c;i++) {
//    if (traffic[i]->_measured_in_flight)
//      fprintf(fp,"   Network %u has %u _measured_in_flight\n", i, traffic[i]->_measured_in_flight );
//  }
//
//  for (unsigned i=0 ;i<(_n_shader+_n_mem);i++ ) {
//    if( !traffic[0]->_partial_packets[i] [0].empty() )
//      fprintf(fp,"   Network 0 has nonempty _partial_packets[%u][0]\n", i);
//    if ( doub_net && !traffic[1]->_partial_packets[i] [0].empty() )
//      fprintf(fp,"   Network 1 has nonempty _partial_packets[%u][0]\n", i);
//    for (unsigned j=0;j<g_num_vcs;j++ ) {
//      if( !ejection_buf[i][j].empty() )
//        fprintf(fp,"   ejection_buf[%u][%u] is non-empty\n", i, j);
//      if( clock_boundary_buf[i][j].has_packet() )
//        fprintf(fp,"   clock_boundary_buf[%u][%u] has packet\n", i, j );
//    }
//  }
}

void InterconnectInterface::Transfer2BoundaryBuffer(int subnet, int output)
{
  Flit* flit;
  int vc;
  for (vc=0; vc<_vcs;vc++) {

    if ( !_ejection_buffer[subnet][output][vc].empty() && _boundary_buffer[subnet][output][vc].Size() < _boundary_buffer_capacity ) {
      flit = _ejection_buffer[subnet][output][vc].front();
      assert(flit);

      _ejection_buffer[subnet][output][vc].pop();
      _boundary_buffer[subnet][output][vc].PushFlitData( flit->data, flit->tail);

      _ejected_flit_queue[subnet][output].push(flit); //indicate this flit is already popped from ejection buffer and ready for credit return

      if ( flit->head ) {
        assert (flit->dest == output);
      }
    }
  }
}

void InterconnectInterface::WriteOutBuffer(int subnet, int output_icntID, Flit*  flit )
{
  int vc = flit->vc;
  assert (_ejection_buffer[subnet][output_icntID][vc].size() < _ejection_buffer_capacity);
  _ejection_buffer[subnet][output_icntID][vc].push(flit);
}

int InterconnectInterface::GetIcntTime() const
{
  return _traffic_manager->getTime();
}

Stats* InterconnectInterface::GetIcntStats(const string &name) const
{
  return _traffic_manager->getStats(name);
}

Flit* InterconnectInterface::GetEjectedFlit(int subnet, int node)
{
  Flit* flit = NULL;
  if (!_ejected_flit_queue[subnet][node].empty()) {
    flit = _ejected_flit_queue[subnet][node].front();
    _ejected_flit_queue[subnet][node].pop();
  }
  return flit;
}

void InterconnectInterface::_CreateBuffer()
{
  unsigned nodes = _n_shader + _n_mem;

  _boundary_buffer.resize(_subnets);
  _ejection_buffer.resize(_subnets);
  _round_robin_turn.resize(_subnets);
  _ejected_flit_queue.resize(_subnets);

  _round_robin_subnet_turn.resize(nodes,0); // jgardea

  for (int subnet = 0; subnet < _subnets; ++subnet) {
    _ejection_buffer[subnet].resize(nodes);
    _boundary_buffer[subnet].resize(nodes);
    _round_robin_turn[subnet].resize(nodes,0);
    _ejected_flit_queue[subnet].resize(nodes);

    for (unsigned node=0;node < nodes;++node){
      _ejection_buffer[subnet][node].resize(_vcs);
      _boundary_buffer[subnet][node].resize(_vcs);
    }
  }
}

void InterconnectInterface::_CreateNodeMap(unsigned n_shader, unsigned n_mem, unsigned n_node, int use_map)
{
  if (use_map) {
    // The (<SM, Memory>, Memory Location Vector) map
    map<pair<unsigned,unsigned>, vector<unsigned> > preset_memory_map;

   // cout << "\t\tShaders " << n_shader << " Memory nodes " << n_mem << " Nodes " << n_node << endl; // jgardea

/* ====================================================================
    I am unsing my own memory map
   ==================================================================== */

//    // preset memory and shader map, optimized for mesh
//    // good for 8 SMs and 8 memory ports, the map is as follows:
//    // +--+--+--+--+
//    // |C0|M0|C1|M1|
//    // +--+--+--+--+
//    // |M2|C2|M3|C3|
//    // +--+--+--+--+
//    // |C4|M4|C5|M5|
//    // +--+--+--+--+
//    // |M6|C6|M7|C7|
//    // +--+--+--+--+
//
//    {
//      unsigned memory_node[] = {1, 3, 4, 6, 9, 11, 12, 14};
//      preset_memory_map[make_pair(8,8)] = vector<unsigned>(memory_node, memory_node+8);
//    }
//
//    // good for 28 SMs and 8 memory ports
//    {
//      unsigned memory_node[] = {3, 7, 10, 12, 23, 25, 28, 32};
//      preset_memory_map[make_pair(28,8)] = vector<unsigned>(memory_node, memory_node+8);
//    }
//
//    // good for 56 SMs and 8 memory cores
//    {
//      unsigned memory_node[] = {3, 15, 17, 29, 36, 47, 49, 61};
//      preset_memory_map[make_pair(56,8)] = vector<unsigned>(memory_node, memory_node+sizeof(memory_node)/sizeof(unsigned));
//    }
//
//    // good for 110 SMs and 11 memory cores
//    {
//      unsigned memory_node[] = {12, 20, 25, 28, 57, 60, 63, 92, 95,100,108};
//      preset_memory_map[make_pair(110, 11)] = vector<unsigned>(memory_node, memory_node+sizeof(memory_node)/sizeof(unsigned));
//    }

    // good for VB-3Dmesh  // jgardea
    {
      unsigned memory_node[n_mem]; 
      unsigned m, i;
      for ( m = n_shader, i = 0; m < n_node; m++, i ++ )
      {
        memory_node[i] = m;
      }
      preset_memory_map[make_pair(n_shader, n_mem )] = vector<unsigned>(memory_node, memory_node+sizeof(memory_node)/sizeof(unsigned)); 
    }

    const vector<int> config_memory_node(_icnt_config->GetIntArray("memory_node_map"));
    
    if (!config_memory_node.empty()) {
      if (config_memory_node.size() != _n_mem) {
        cerr << "Number of memory nodes in memory_node_map should equal to memory ports" << endl;
        assert( config_memory_node.size() == _n_mem);
      }
      vector<unsigned> t_memory_node(config_memory_node.size());
      copy(config_memory_node.begin(), config_memory_node.end(), t_memory_node.begin());
      preset_memory_map[make_pair(_n_shader, _n_mem)] = t_memory_node;
    }
    
    const vector<unsigned> &memory_node = preset_memory_map[make_pair(_n_shader, _n_mem)];
    if (memory_node.empty()) {
      cerr<<"ERROR!!! NO MAPPING IMPLEMENTED YET FOR THIS CONFIG"<<endl;
      assert(0);
    }

    // create node map
    unsigned next_node = 0;
    unsigned memory_node_index = 0;
    for (unsigned i = 0; i < n_shader; ++i) {
      while (next_node == memory_node[memory_node_index]) {
        next_node += 1;
        memory_node_index += 1;
      }
      _node_map[i] = next_node;
      next_node += 1;
    }
    for (unsigned i = n_shader; i < n_shader+n_mem; ++i) {
      _node_map[i] = memory_node[i-n_shader];
    }
  } else { //not use preset map
    for (unsigned i=0;i<n_node;i++) {
      _node_map[i]=i;
    }
  }

  for (unsigned i = 0; i < n_node ; i++) {
    for (unsigned j = 0; j< n_node ; j++) {
      if ( _node_map[j] == i ) {
        _reverse_node_map[i]=j;
        break;
      }
    }
  }

  //FIXME: should compatible with non-square number
  //_DisplayMap((int) sqrt(n_node), n_node);

}

void InterconnectInterface::_DisplayMap(int dim,int count)
{
  cout << "GPGPU-Sim uArch: interconnect node map (shaderID+MemID to icntID)" << endl;
  cout << "GPGPU-Sim uArch: Memory nodes ID start from index: " << _n_shader << endl;
  cout << "GPGPU-Sim uArch: ";
  for (int i = 0;i < count; i++) {
    cout << setw(4) << _node_map[i];
    if ((i+1)%dim == 0 && i != count-1)
      cout << endl << "GPGPU-Sim uArch: ";
  }
  cout << endl;

  cout << "GPGPU-Sim uArch: interconnect node reverse map (icntID to shaderID+MemID)" << endl;
  cout << "GPGPU-Sim uArch: Memory nodes start from ID: " << _n_shader << endl;
  cout << "GPGPU-Sim uArch: ";
  for (int i = 0;i < count; i++) {
    cout << setw(4) << _reverse_node_map[i];
    if ((i+1)%dim == 0 && i != count-1)
      cout << endl << "GPGPU-Sim uArch: ";
  }
  cout << endl;
}

void* InterconnectInterface::_BoundaryBufferItem::PopPacket()
{
  assert (_packet_n);
  void * data = NULL;
  void * flit_data = _buffer.front();
  while (data == NULL) {
    assert(flit_data == _buffer.front()); //all flits must belong to the same packet
    if (_tail_flag.front()) {
      data = _buffer.front();
      _packet_n--;
    }
    _buffer.pop();
    _tail_flag.pop();
  }
  return data;
}

void* InterconnectInterface::_BoundaryBufferItem::TopPacket() const
{
  assert (_packet_n);
  void* data = NULL;
  void* temp_d = _buffer.front();
  while (data==NULL) {
    if (_tail_flag.front()) {
      data = _buffer.front();
    }
    assert(temp_d == _buffer.front()); //all flits must belong to the same packet
  }
  return data;

}

void InterconnectInterface::_BoundaryBufferItem::PushFlitData(void* data,bool is_tail)
{
  _buffer.push(data);
  _tail_flag.push(is_tail);
  if (is_tail) {
    _packet_n++;
  }
}
