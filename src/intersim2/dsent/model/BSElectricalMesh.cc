#include "model/BSElectricalMesh.h"

#include <cmath>

#include "model/PortInfo.h"
#include "model/EventInfo.h"
#include "model/TransitionInfo.h"
#include "model/ModelGen.h"
#include "std_cells/StdCellLib.h"
#include "timing_graph/ElectricalTimingTree.h"
#include "timing_graph/ElectricalNet.h"

namespace DSENT
{
    using std::sqrt;

    BSElectricalMesh::BSElectricalMesh(const String& instance_name_, const TechModel* tech_model_)
        : ElectricalModel(instance_name_, tech_model_)
    {
        initParameters();
        initProperties();
    }

    BSElectricalMesh::~BSElectricalMesh()
    {}

    void BSElectricalMesh::initParameters()
    {
        // Clock Frequency
        addParameterName("Frequency");
        // Physical Parameters
        addParameterName("NumberBitsPerFlit");
        addParameterName("NumberBitsPerFlitAsym"); // jgardea
        addParameterName("AsymmetricNetwork");
        addParameterName("VerticalBus");
        // Router parameters
        addParameterName("Router->NumberPorts");
        addParameterName("Router->NumberVirtualNetworks");
        addParameterName("Router->NumberVirtualChannelsPerVirtualNetwork");
        addParameterName("Router->NumberBuffersPerVirtualChannel");
        addParameterName("Router->InputPort->BufferModel");
        addParameterName("Router->CrossbarModel");
        addParameterName("Router->SwitchAllocator->ArbiterModel");
        addParameterName("Router->ClockTreeModel");
        addParameterName("Router->ClockTree->NumberLevels");
        addParameterName("Router->ClockTree->WireLayer");
        addParameterName("Router->ClockTree->WireWidthMultiplier");
        addParameterName("Router->ClockTree->WireSpacingMultiplier", 3.0);
        // Link parameters
        addParameterName("Link->WireLayer");
        addParameterName("Link->WireWidthMultiplier");
        addParameterName("Link->WireSpacingMultiplier");

        return;
    }

    void BSElectricalMesh::initProperties()
    {
        addPropertyName("SitePitch");
        addPropertyName("VerticalLinkLength");  // jgardea
        return;
    }

    void BSElectricalMesh::constructModel()
    {
        setAsymmetric(getParameter("AsymmetricNetwork").toBool());
        hasVerticalBus = getParameter("VerticalBus").toBool();

        // Get input paramters
        unsigned int number_bits_per_flit = getParameter("NumberBitsPerFlit").toUInt();
        
        ASSERT(number_bits_per_flit > 0, "[Error] " + getInstanceName() + 
                " -> Number of bits per flit must be > 0!");
        
        // Get input parameters that will be forwarded to the sub instances
        const String& router_number_vns = getParameter("Router->NumberVirtualNetworks");
        const String& router_number_vcs_per_vn = getParameter("Router->NumberVirtualChannelsPerVirtualNetwork");
        const String& router_number_bufs_per_vc = getParameter("Router->NumberBuffersPerVirtualChannel");
        const String& link_wire_layer = getParameter("Link->WireLayer");
        const String& link_wire_width_multiplier = getParameter("Link->WireWidthMultiplier");
        const String& link_wire_spacing_multiplier = getParameter("Link->WireSpacingMultiplier");
        unsigned  router_ports = getParameter("Router->NumberPorts").toUInt();

        //getGenProperties()->set("NumberRouters", 1);
        //getGenProperties()->set("NumberLinks", 1);
        //getGenProperties()->set("NumberRouterToSiteLinks", 1);
        getGenProperties()->set("Router->NumberInputPorts", router_ports );
        getGenProperties()->set("Router->NumberOutputPorts", router_ports );

        // Create ports
        createInputPort("CK");

        // Init router model
        ElectricalModel* router = (ElectricalModel*)ModelGen::createModel("Router", "MeshRouter", getTechModel());
        router->setParameter("NumberInputPorts", router_ports);
        router->setParameter("NumberOutputPorts", router_ports);
        router->setParameter("NumberVirtualNetworks", router_number_vns);
        router->setParameter("NumberVirtualChannelsPerVirtualNetwork", router_number_vcs_per_vn);
        router->setParameter("NumberBuffersPerVirtualChannel", router_number_bufs_per_vc);
        router->setParameter("NumberBitsPerFlit", number_bits_per_flit);
        router->setParameter("InputPort->BufferModel", getParameter("Router->InputPort->BufferModel"));
        router->setParameter("CrossbarModel", getParameter("Router->CrossbarModel"));
        router->setParameter("SwitchAllocator->ArbiterModel", getParameter("Router->SwitchAllocator->ArbiterModel"));
        router->setParameter("ClockTreeModel", getParameter("Router->ClockTreeModel"));
        router->setParameter("ClockTree->NumberLevels", getParameter("Router->ClockTree->NumberLevels"));
        router->setParameter("ClockTree->WireLayer", getParameter("Router->ClockTree->WireLayer"));
        router->setParameter("ClockTree->WireWidthMultiplier", getParameter("Router->ClockTree->WireWidthMultiplier"));
        router->setParameter("ClockTree->WireSpacingMultiplier", getParameter("Router->ClockTree->WireSpacingMultiplier"));
        router->construct();

        // Init router to router links
        ElectricalModel* rr_link = (ElectricalModel*)ModelGen::createModel("RepeatedLink", "Link", getTechModel());
        rr_link->setParameter("NumberBits", number_bits_per_flit);
        rr_link->setParameter("WireLayer", link_wire_layer);
        rr_link->setParameter("WireWidthMultiplier", link_wire_width_multiplier);
        rr_link->setParameter("WireSpacingMultiplier", link_wire_spacing_multiplier);
        rr_link->construct();

        ElectricalModel* ver_link = (ElectricalModel*) ModelGen::createModel("RepeatedLink", "VerticalLink", getTechModel());
        ver_link->setParameter("NumberBits", number_bits_per_flit);
        ver_link->setParameter("WireLayer", link_wire_layer);
        ver_link->setParameter("WireWidthMultiplier", link_wire_width_multiplier);
        ver_link->setParameter("WireSpacingMultiplier", link_wire_spacing_multiplier);
        ver_link->construct(); 

        // Connect ports
        createNet("RR_Link_Out", makeNetIndex(0, number_bits_per_flit-1));
        createNet("RR_Link_In", makeNetIndex(0, number_bits_per_flit-1));
        portConnect(rr_link, "In", "RR_Link_In");
        portConnect(rr_link, "Out", "RR_Link_Out");

        createNet("Ver_Link_Out", makeNetIndex(0, number_bits_per_flit-1));
        createNet("Ver_Link_In", makeNetIndex(0, number_bits_per_flit-1));
        portConnect(ver_link, "In", "Ver_Link_In");
        portConnect(ver_link, "Out", "Ver_Link_Out");

        portConnect(router, "CK", "CK");

        unsigned rr_ports = (hasVerticalBus) ? router_ports-2 : router_ports ;

        for(unsigned int i = 0; i < router_ports; ++i) 
        {
            createNet("Router_In" + (String)i, makeNetIndex(0, number_bits_per_flit-1));
            portConnect(router, "FlitIn" + (String)i, "Router_In" + (String)i);
        }
        for(unsigned int i = 0; i < router_ports; ++i)
        {
            createNet("Router_Out" + (String)i, makeNetIndex(0, number_bits_per_flit-1));
            portConnect(router, "FlitOut" + (String)i, "Router_Out" + (String)i);
        }
        for(unsigned int i = 0; i < number_bits_per_flit; ++i)
        {
            unsigned int j;
            for(j = 0; j < rr_ports; ++j)
            {
                assignVirtualFanout("Router_In" + (String)j, makeNetIndex(i), "RR_Link_Out", makeNetIndex(i));
                assignVirtualFanin("RR_Link_In", makeNetIndex(i), "Router_Out" + (String)j, makeNetIndex(i));
            }

            if ( hasVerticalBus )
            {
                for(j = rr_ports; j < router_ports; j++ )
                {
                    assignVirtualFanout("Router_In" + (String)j, makeNetIndex(i), "Ver_Link_Out", makeNetIndex(i));
                    assignVirtualFanin("Ver_Link_In", makeNetIndex(i), "Router_Out" + (String)j, makeNetIndex(i)); 
                }
            }
        }

        // Create area, power and event results
        createElectricalResults();
        createElectricalEventResult("Flit");

        // Add all instances
        addSubInstances(router, 1);
        addElectricalSubResults(router, 1);
        addSubInstances(rr_link, 1);
        addElectricalSubResults(rr_link, 1);
        addSubInstances(ver_link, 1);
        addElectricalSubResults(ver_link, 1);
        
        Result* energy_per_flit = getEventResult("Flit");
        energy_per_flit->addSubResult(rr_link->getEventResult("Send"), "Link", 1);
        energy_per_flit->addSubResult(ver_link->getEventResult("Send"), "VerticalLink", 1);
        energy_per_flit->addSubResult(router->getEventResult("WriteBuffer"), "MeshRouter", 1);
        energy_per_flit->addSubResult(router->getEventResult("ReadBuffer"), "MeshRouter", 1);
        energy_per_flit->addSubResult(router->getEventResult("TraverseCrossbar->Multicast1"), "MeshRouter", 1);
        energy_per_flit->addSubResult(router->getEventResult("ArbitrateSwitch->ArbitrateStage1"), "MeshRouter", 1);
        energy_per_flit->addSubResult(router->getEventResult("ArbitrateSwitch->ArbitrateStage2"), "MeshRouter", 1);
        //energy_per_flit->addSubResult(router->getEventResult("DistributeClock"), "MeshRouter", 1);

        if ( isAsymmetric() )
        {
            unsigned int number_bits_per_flit_asym = getParameter("NumberBitsPerFlitAsym").toUInt();
            ASSERT(number_bits_per_flit_asym > 0, "[Error] " + getInstanceName() + 
                " -> Number of bits per flit must be > 0!");
            
            ElectricalModel* asym_router = (ElectricalModel*)ModelGen::createModel("Router", "AsymMeshRouter", getTechModel());
            asym_router->setParameter("NumberInputPorts", router_ports);
            asym_router->setParameter("NumberOutputPorts", router_ports);
            asym_router->setParameter("NumberVirtualNetworks", router_number_vns);
            asym_router->setParameter("NumberVirtualChannelsPerVirtualNetwork", router_number_vcs_per_vn);
            asym_router->setParameter("NumberBuffersPerVirtualChannel", router_number_bufs_per_vc);
            asym_router->setParameter("NumberBitsPerFlit", number_bits_per_flit_asym);
            asym_router->setParameter("InputPort->BufferModel", getParameter("Router->InputPort->BufferModel"));
            asym_router->setParameter("CrossbarModel", getParameter("Router->CrossbarModel"));
            asym_router->setParameter("SwitchAllocator->ArbiterModel", getParameter("Router->SwitchAllocator->ArbiterModel"));
            asym_router->setParameter("ClockTreeModel", getParameter("Router->ClockTreeModel"));
            asym_router->setParameter("ClockTree->NumberLevels", getParameter("Router->ClockTree->NumberLevels"));
            asym_router->setParameter("ClockTree->WireLayer", getParameter("Router->ClockTree->WireLayer"));
            asym_router->setParameter("ClockTree->WireWidthMultiplier", getParameter("Router->ClockTree->WireWidthMultiplier"));
            asym_router->setParameter("ClockTree->WireSpacingMultiplier", getParameter("Router->ClockTree->WireSpacingMultiplier"));
            asym_router->construct();

            ElectricalModel* asym_rr_link = (ElectricalModel*)ModelGen::createModel("RepeatedLink", "AsymLink", getTechModel());
            asym_rr_link->setParameter("NumberBits", number_bits_per_flit_asym);
            asym_rr_link->setParameter("WireLayer", link_wire_layer);
            asym_rr_link->setParameter("WireWidthMultiplier", link_wire_width_multiplier);
            asym_rr_link->setParameter("WireSpacingMultiplier", link_wire_spacing_multiplier);
            asym_rr_link->construct();

            ElectricalModel* asym_ver_link = (ElectricalModel*) ModelGen::createModel("RepeatedLink", "AsymVerticalLink", getTechModel());
            asym_ver_link->setParameter("NumberBits", number_bits_per_flit_asym);
            asym_ver_link->setParameter("WireLayer", link_wire_layer);
            asym_ver_link->setParameter("WireWidthMultiplier", link_wire_width_multiplier);
            asym_ver_link->setParameter("WireSpacingMultiplier", link_wire_spacing_multiplier);
            asym_ver_link->construct();

            // Connect ports
            createNet("Asym_RR_Link_Out", makeNetIndex(0, number_bits_per_flit_asym-1));
            createNet("Asym_RR_Link_In", makeNetIndex(0, number_bits_per_flit_asym-1));
            portConnect(asym_rr_link, "In", "Asym_RR_Link_In");
            portConnect(asym_rr_link, "Out", "Asym_RR_Link_Out");

            createNet("Asym_Ver_Link_Out", makeNetIndex(0, number_bits_per_flit_asym-1));
            createNet("Asym_Ver_Link_In", makeNetIndex(0, number_bits_per_flit_asym-1));
            portConnect(asym_ver_link, "In", "Asym_Ver_Link_In");
            portConnect(asym_ver_link, "Out", "Asym_Ver_Link_Out");

            portConnect(asym_router, "CK", "CK");

            for(unsigned int i = 0; i < router_ports; ++i)  
            {
                createNet("Asym_Router_In" + (String)i, makeNetIndex(0, number_bits_per_flit_asym-1));
                portConnect(asym_router, "FlitIn" + (String)i, "Asym_Router_In" + (String)i);
            }
            for(unsigned int i = 0; i < router_ports; ++i)
            {
                createNet("Asym_Router_Out" + (String)i, makeNetIndex(0, number_bits_per_flit_asym-1));
                portConnect(asym_router, "FlitOut" + (String)i, "Asym_Router_Out" + (String)i);
            }
            for(unsigned int i = 0; i < number_bits_per_flit_asym; ++i)
            {
                unsigned int j;
                for(j = 0; j < rr_ports; ++j)
                {
                    assignVirtualFanout("Asym_Router_In" + (String)j, makeNetIndex(i), "Asym_RR_Link_Out", makeNetIndex(i));
                    assignVirtualFanin("Asym_RR_Link_In", makeNetIndex(i), "Asym_Router_Out" + (String)j, makeNetIndex(i));
                }

                if ( hasVerticalBus )
                {
                    for(j = rr_ports; j < router_ports; j++ )
                    {
                        assignVirtualFanout("Asym_Router_In" + (String)j, makeNetIndex(i), "Asym_Ver_Link_Out", makeNetIndex(i));
                        assignVirtualFanin("Asym_Ver_Link_In", makeNetIndex(i), "Asym_Router_Out" + (String)j, makeNetIndex(i)); 
                    }
                }
            }

            addSubInstances(asym_router, 1);
            addElectricalSubResults(asym_router, 1);
            addSubInstances(asym_rr_link, 1);
            addElectricalSubResults(asym_rr_link, 1);
            addSubInstances(asym_ver_link, 1);
            addElectricalSubResults(asym_ver_link, 1);

            energy_per_flit->addSubResult(asym_rr_link->getEventResult("Send"), "AsymLink", 1);
            energy_per_flit->addSubResult(asym_ver_link->getEventResult("Send"), "AsymVerticalLink", 1);
            energy_per_flit->addSubResult(asym_router->getEventResult("WriteBuffer"), "AsymMeshRouter", 1);
            energy_per_flit->addSubResult(asym_router->getEventResult("ReadBuffer"), "AsymMeshRouter", 1);
            energy_per_flit->addSubResult(asym_router->getEventResult("TraverseCrossbar->Multicast1"), "AsymMeshRouter", 1);
            energy_per_flit->addSubResult(asym_router->getEventResult("ArbitrateSwitch->ArbitrateStage1"), "AsymMeshRouter", 1);
            energy_per_flit->addSubResult(asym_router->getEventResult("ArbitrateSwitch->ArbitrateStage2"), "AsymMeshRouter", 1);
            //energy_per_flit->addSubResult(asym_router->getEventResult("DistributeClock"), "AsymMeshRouter", 1);
        }
        
        return;
    }

    void BSElectricalMesh::updateModel()
    {
        // Get properties
        double site_pitch = getProperty("SitePitch").toDouble();
        double clock_freq = getParameter("Frequency");

        ASSERT(site_pitch > 0, "[Error] " + getInstanceName() + 
                " -> Site pitch must be > 0!");
        ASSERT(clock_freq > 0, "[Error] " + getInstanceName() +
                " -> Clock frequency must be > 0!");

        // Get margin on link delays, since there are registers before and after the link
        double delay_ck_to_q = getTechModel()->getStdCellLib()->getStdCellCache()->get("DFFQ_X1->Delay->CK_to_Q");
        double delay_setup = getTechModel()->getStdCellLib()->getStdCellCache()->get("DFFQ_X1->Delay->CK_to_Q");
        double link_delay_margin = (delay_ck_to_q + delay_setup) * 1.5;
        
        double rr_link_length = site_pitch;
        double rr_link_delay = std::max(1e-99, 1.0 / clock_freq - link_delay_margin);
        double router_delay = 1.0 / clock_freq;

        double ver_link_length = getProperty("VerticalLinkLength");

        Model* rr_link = getSubInstance("Link");
        rr_link->setProperty("WireLength", rr_link_length);
        rr_link->setProperty("Delay", rr_link_delay);
        rr_link->setProperty("IsKeepParity", "TRUE");
        rr_link->update();
        
        Model* ver_link = getSubInstance("VerticalLink");
        ver_link->setProperty("WireLength", ver_link_length);
        ver_link->setProperty("Delay", rr_link_delay);
        ver_link->setProperty("IsKeepParity", "TRUE");
        ver_link->update();

        ElectricalModel* router = (ElectricalModel*)getSubInstance("MeshRouter");
        router->update();

        ElectricalTimingTree router_timing_tree("MeshRouter", router);
        router_timing_tree.performTimingOpt(router->getNet("CK"), router_delay);

        if ( isAsymmetric() )
        {
            Model* asym_rr_link = getSubInstance("AsymLink");
            asym_rr_link->setProperty("WireLength", rr_link_length);
            asym_rr_link->setProperty("Delay", rr_link_delay);
            asym_rr_link->setProperty("IsKeepParity", "TRUE");
            asym_rr_link->update();

            Model* asym_ver_link = getSubInstance("AsymVerticalLink");
            asym_ver_link->setProperty("WireLength", ver_link_length);
            asym_ver_link->setProperty("Delay", rr_link_delay);
            asym_ver_link->setProperty("IsKeepParity", "TRUE");
            asym_ver_link->update();

            ElectricalModel* asym_router = (ElectricalModel*)getSubInstance("AsymMeshRouter");
            asym_router->update();

            ElectricalTimingTree asym_router_timing_tree("AsymMeshRouter", asym_router);
            asym_router_timing_tree.performTimingOpt(asym_router->getNet("CK"), router_delay);
        }

        return;
    }

    void BSElectricalMesh::propagateTransitionInfo()
    {
        // Get parameters
        unsigned int router_number_input_ports = getGenProperties()->get("Router->NumberInputPorts");

        ElectricalModel* rr_link = (ElectricalModel*)getSubInstance("Link");
        assignPortTransitionInfo(rr_link, "In", TransitionInfo(0.25, 0.25, 0.25));
        rr_link->use();

        ElectricalModel* ver_link = (ElectricalModel*)getSubInstance("VerticalLink");
        assignPortTransitionInfo(ver_link, "In", TransitionInfo(0.25, 0.25, 0.25));
        ver_link->use();

        ElectricalModel* router = (ElectricalModel*)getSubInstance("MeshRouter");
        for(unsigned int i = 0; i < router_number_input_ports; ++i)
        {
            assignPortTransitionInfo(router, "FlitIn" + (String)i, TransitionInfo(0.25, 0.25, 0.25));
        }
        assignPortTransitionInfo(router, "CK", TransitionInfo(0.0, 1.0, 0.0));
        router->getGenProperties()->set("UseModelEvent", "");
        router->use();

        if ( isAsymmetric() )
        {
            ElectricalModel* asym_rr_link = (ElectricalModel*)getSubInstance("AsymLink");
            assignPortTransitionInfo(asym_rr_link, "In", TransitionInfo(0.25, 0.25, 0.25));
            asym_rr_link->use();

            ElectricalModel* asym_ver_link = (ElectricalModel*)getSubInstance("AsymVerticalLink");
            assignPortTransitionInfo(asym_ver_link, "In", TransitionInfo(0.25, 0.25, 0.25));
            asym_ver_link->use();

            ElectricalModel* asym_router = (ElectricalModel*)getSubInstance("AsymMeshRouter");
            for(unsigned int i = 0; i < router_number_input_ports; ++i)
            {
                assignPortTransitionInfo(asym_router, "FlitIn" + (String)i, TransitionInfo(0.25, 0.25, 0.25));
            }
            assignPortTransitionInfo(asym_router, "CK", TransitionInfo(0.0, 1.0, 0.0));
            asym_router->getGenProperties()->set("UseModelEvent", "");
            asym_router->use();
        }

        return;
    }
} // namespace DSENT

