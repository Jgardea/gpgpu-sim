#include "electrical/TSV.h"

#include "model/PortInfo.h"
#include "model/EventInfo.h"
#include "model/TransitionInfo.h"
#include "std_cells/StdCellLib.h"
#include "std_cells/StdCell.h"
#include "timing_graph/ElectricalTimingTree.h"
#include "timing_graph/ElectricalTimingNode.h"
#include "timing_graph/ElectricalNet.h"
#include "timing_graph/ElectricalDriver.h"
#include "timing_graph/ElectricalDelay.h"
#include "timing_graph/ElectricalLoad.h"

namespace DSENT
{
    TSV::TSV(const String& instance_name_, const TechModel* tech_model_)
        : RepeatedLink(instance_name_, tech_model_)
    {
		m_repeater_ = NULL;
		m_repeater_load_ = NULL;
		m_timing_tree_ = NULL;
        initParameters();
        initProperties();
    }

    TSV::~TSV()
    {
        delete m_repeater_;
        delete m_repeater_load_;
        delete m_timing_tree_;
    }

    void TSV::initParameters()
    {
        addParameterName("WireLayer", "Vertical");
		addParameterName("Frequency");
        return;
    }

    void TSV::initProperties()
    {
		addPropertyName("WireLength",0);
        addPropertyName("Length");
		addPropertyName("Diameter");
		addPropertyName("Pitch");
		addPropertyName("BumpDiameter");
        return;
    }

    TSV* TSV::clone() const
    {
        // TODO
        return NULL;
    }

    void TSV::constructModel()
    {
        // Get parameters
        unsigned int number_bits = getParameter("NumberBits").toUInt();

        ASSERT(number_bits > 0, "[Error] " + getInstanceName() + 
                " -> Number of bits must be > 0!");

        // Create ports
        createInputPort("In", makeNetIndex(0, number_bits-1));
        createOutputPort("Out", makeNetIndex(0, number_bits-1));

        // Create area, power, and event results
        createElectricalAtomicResults();   
        createElectricalEventAtomicResult("Send");

        // Create connections
        // Since the length is not set yet, we only to virtual fan-in and virtual fan-out
        createNet("InTmp");
        createNet("OutTmp");
        assignVirtualFanin("InTmp", "In");
        assignVirtualFanout("Out", "OutTmp");

        // Build Electrical Connectivity
        createLoad("In_Cap");
        createDelay("In_to_Out_delay");
        createDriver("Out_Ron", false); // Indicate this driver is not sizable

        ElectricalLoad* in_cap = getLoad("In_Cap");
        ElectricalDelay* in_to_out_delay = getDelay("In_to_Out_delay");
        ElectricalDriver* out_ron = getDriver("Out_Ron");

        getNet("InTmp")->addDownstreamNode(in_cap);
        in_cap->addDownstreamNode(in_to_out_delay);
        in_to_out_delay->addDownstreamNode(out_ron);
        out_ron->addDownstreamNode(getNet("OutTmp"));

        // Init a repeater and a load to mimic a segment of a repeated link
        m_repeater_ = getTechModel()->getStdCellLib()->createStdCell("INV", "Repeater");
        m_repeater_->construct();
        m_repeater_load_ = new ElectricalLoad("RepeaterIn_Cap", this);
        // Make path repeater_ -> repeater_load_
        // to catch the repeater's input/output cap and ensure only one inverter delay
        // is added
        m_repeater_->getNet("Y")->addDownstreamNode(m_repeater_load_);
        // Init a timing object to calculate delay
        m_timing_tree_ = new ElectricalTimingTree("TSV", this);
        m_timing_tree_->performCritPathExtract(m_repeater_->getNet("A"));
        return;
    }

    void TSV::updateModel()
    {
        unsigned int number_bits = getParameter("NumberBits").toUInt();
		double freq = getParameter("Frequency");

        // Get properties
		double tsv_length = getProperty("Length").toDouble();
		double tsv_diameter = getProperty("Diameter").toDouble();
		double tsv_pitch = getProperty("Pitch").toDouble();
		double bump_diameter = getProperty("BumpDiameter").toDouble();
        double required_delay = getProperty("Delay").toDouble();
        bool isKeepParity = getProperty("IsKeepParity").toBool();

		ASSERT(tsv_length >= 0, "[Error] " + getInstanceName() +
                " -> TSV length must be >= 0!");
        ASSERT(required_delay >= 0, "[Error] " + getInstanceName() + 
                " -> Required delay must be >= 0!");

		getGenProperties()->set("TSVLength", tsv_length);
		getGenProperties()->set("TSVDiameter", tsv_diameter);
		getGenProperties()->set("TSVPitch", tsv_pitch);
		getGenProperties()->set("BumpDiameter", bump_diameter);

		// TSV is calculated taking in consideration TSV Bump, Silicon capacitance and Silicon Dioxide Capacitance
		// TSV Resistance is calculated taking in consideration frequency
		// All properties are assumed in a 100 °C environment

		double cap_bump = getTechModel()->calculateBumpCapacitance( bump_diameter/2, tsv_diameter/2);
		double silicon_res = getTechModel()->calculateSiliconResistance(tsv_pitch, tsv_diameter, tsv_length);
		double silicon_cap = getTechModel()->calculateSiliconCapacitance(tsv_length, tsv_pitch, tsv_diameter);
		double tsv_res = getTechModel()->calculateTSVResistance(silicon_res, silicon_cap, tsv_diameter, tsv_length, freq);
		double tsv_cap = getTechModel()->calculateTSVCapacitance(silicon_res, silicon_cap, tsv_diameter/2, tsv_length, freq);
		tsv_cap += cap_bump;
		double total_tsv_res = tsv_res * number_bits;
		double total_tsv_cap = tsv_cap * number_bits;

        m_repeater_->update();

        unsigned int increment_segments = (isKeepParity)? 2:1;
        unsigned int number_segments = increment_segments;
        double delay;
        m_repeater_->setMinDrivingStrength();
        m_repeater_->getNet("Y")->setDistributedCap(total_tsv_cap / number_segments);
        m_repeater_->getNet("Y")->setDistributedRes(total_tsv_res / number_segments);
        m_repeater_load_->setLoadCap(m_repeater_->getNet("A")->getTotalDownstreamCap());
        m_timing_tree_->performCritPathExtract(m_repeater_->getNet("A"));
        delay = m_timing_tree_->calculateCritPathDelay(m_repeater_->getNet("A")) * number_segments;

        // If everything is 0, use number_segments min-sized repeater
        if(tsv_length != 0)
        {
            // Set the initial number of segments based on isKeepParity
            double last_min_size_delay = 0;
            unsigned int iteration = 0;

            // First set the repeater to the minimum driving strength
            last_min_size_delay = delay;

            Log::printLine(getInstanceName() + " -> Beginning Repeater Insertion");

            while(required_delay < delay)
            {
				//cout << "delay: " << delay << endl;
                Log::printLine(getInstanceName() + " -> Repeater Insertion Iteration " + (String)iteration + 
                        ": Required delay = " + (String)required_delay + 
                        ", Delay = " + (String)delay + 
                        ", Slack = " + (String)(required_delay - delay) + 
                        ", Number of repeaters = " + (String)number_segments);

                // Size up if timing is not met
                while(required_delay < delay)
                {
                    if(m_repeater_->hasMaxDrivingStrength())
                    {
                        break;
                    }
                    m_repeater_->increaseDrivingStrength();
                    m_repeater_load_->setLoadCap(m_repeater_->getNet("A")->getTotalDownstreamCap());
                    m_timing_tree_->performCritPathExtract(m_repeater_->getNet("A"));
                    delay = m_timing_tree_->calculateCritPathDelay(m_repeater_->getNet("A")) * number_segments;

                    iteration++;
                    Log::printLine(getInstanceName() + " -> Slack: " + (String)(required_delay - delay));
                }
                // Increase number of segments if timing is not met
                if(required_delay < delay)
                {
                    number_segments += increment_segments;
                    m_repeater_->setMinDrivingStrength();
                    m_repeater_->getNet("Y")->setDistributedCap(total_tsv_cap / number_segments);
                    m_repeater_->getNet("Y")->setDistributedRes(total_tsv_res / number_segments);
                    m_repeater_load_->setLoadCap(m_repeater_->getNet("A")->getTotalDownstreamCap());
                    m_timing_tree_->performCritPathExtract(m_repeater_->getNet("A"));
                    delay = m_timing_tree_->calculateCritPathDelay(m_repeater_->getNet("A")) * number_segments;

                    // Abort if adding more min sized repeaters does not decrease the delay
                    if(delay > last_min_size_delay)
                    {
                        break;
                    }
                    last_min_size_delay = delay;
                }
            }
            Log::printLine(getInstanceName() + " -> Repeater Insertion Ended after Iteration: " + (String)iteration + 
                    ": Required delay = " + (String)required_delay + 
                    ", Delay = " + (String)delay + 
                    ", Slack = " + (String)(required_delay - delay) + 
                    ", Number of repeaters = " + (String)number_segments);

            // Print a warning if the timing is not met
            if(required_delay < delay)
            {
                const String& warning_msg = "[Warning] " + getInstanceName() + " -> Timing not met" + 
                    ": Required delay = " + (String)required_delay + 
                    ", Delay = " + (String)delay + 
                    ", Slack = " + (String)(required_delay - delay) +
                    ", Number of repeaters = " + (String)number_segments;
                Log::printLine(std::cerr, warning_msg);
            }
        }

		cout << "TSV | Total Res = " << total_tsv_res << endl;
		cout << "TSV | Total Cap = " << total_tsv_cap << " | " << number_segments << endl;

        // Update electrical interfaces
        getLoad("In_Cap")->setLoadCap(m_repeater_->getNet("A")->getTotalDownstreamCap());
        getDelay("In_to_Out_delay")->setDelay(delay);
        getDriver("Out_Ron")->setOutputRes(m_repeater_->getDriver("Y_Ron")->getOutputRes() + (total_tsv_res / number_segments));

        getGenProperties()->set("NumberSegments", number_segments);

        // Update area, power results
        resetElectricalAtomicResults();
        addElecticalAtomicResultValues(m_repeater_, number_segments * number_bits);
       	//double wire_area = wire_length * (wire_width + wire_spacing) * number_bits;
        //addElecticalWireAtomicResultValue(wire_layer, wire_area);

        return;
    }

    void TSV::useModel()
    {
        // Update the transition information for the modeled repeater
        // Since we only modeled one repeater. So the transition information for 0->0 and 1->1 
        // is averaged out
        const TransitionInfo& trans_In = getInputPort("In")->getTransitionInfo();
        double average_static_transition = (trans_In.getNumberTransitions00() + trans_In.getNumberTransitions11()) / 2.0;
        TransitionInfo mod_trans_In(average_static_transition, trans_In.getNumberTransitions01(), average_static_transition);
        m_repeater_->getInputPort("A")->setTransitionInfo(mod_trans_In);
        m_repeater_->use();

        // Get parameters
        unsigned int number_bits = getParameter("NumberBits").toUInt();
        unsigned int number_segments = getGenProperties()->get("NumberSegments").toUInt();

        // Propagate the transition information
        propagateTransitionInfo();

        // Update leakage power
        double power = 0.0;
        power += m_repeater_->getNddPowerResult("Leakage")->calculateSum() * number_segments * number_bits;
        getNddPowerResult("Leakage")->setValue(power);

        // Update event result
        double energy = 0.0;
        energy += m_repeater_->getEventResult("INV")->calculateSum() * number_segments * number_bits;
        getEventResult("Send")->setValue(energy);

        return;
    }

    void TSV::propagateTransitionInfo()
    {
        unsigned int number_segments = getGenProperties()->get("NumberSegments");

        if((number_segments % 2) == 0)
        {
            propagatePortTransitionInfo("Out", "In");
        }
        else
        {
            const TransitionInfo& trans_In = getInputPort("In")->getTransitionInfo();
            TransitionInfo trans_Out(trans_In.getNumberTransitions11(), trans_In.getNumberTransitions01(), trans_In.getNumberTransitions00());
            getOutputPort("Out")->setTransitionInfo(trans_Out);
        }
        return;
    }

} // namespace DSENT

