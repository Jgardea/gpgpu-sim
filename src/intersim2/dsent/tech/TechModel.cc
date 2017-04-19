#include "tech/TechModel.h"

#include <cmath>

#include "std_cells/StdCellLib.h"

namespace DSENT
{
	const double PI = 3.14159265358979323846;
    TechModel::TechModel()
        : Config(), m_std_cell_lib_(NULL), m_available_wire_layers_(NULL)
    {}

    TechModel::~TechModel()
    {}

    void TechModel::setStdCellLib(const StdCellLib* std_cell_lib_)
    {
        m_std_cell_lib_ = std_cell_lib_;
        return;
    }

    const StdCellLib* TechModel::getStdCellLib() const
    {
        return m_std_cell_lib_;
    }

    TechModel* TechModel::clone() const
    {
        return new TechModel(*this);
    }

    void TechModel::readFile(const String& filename_)
    {
        // Read the main technology file
        LibUtil::Config::readFile(filename_);

        // Search for "INCLUDE" to include more technology files
        StringMap::ConstIterator it;
        for(it = begin(); it != end(); ++it)
        {
            const String& key = it->first;
            if(key.compare(0, 8, "INCLUDE_") == 0)
            {
                const String& include_filename = it->second;
                LibUtil::Config::readFile(include_filename);
            }
        }

        // Set the available wire layers
        const vector<String>& available_wire_layer_vector = get("Wire->AvailableLayers").split("[,]");
        m_available_wire_layers_ = new std::set<String>;
        for(unsigned int i = 0; i < available_wire_layer_vector.size(); ++i)
        {
            m_available_wire_layers_->insert(available_wire_layer_vector[i]);
        }
        return;
    }

    //-------------------------------------------------------------------------
    // Transistor Related Functions
    //-------------------------------------------------------------------------
    //Returns the leakage current of NMOS transistors, given the transistor stakcing, transistor widths, and input combination
    double TechModel::calculateNmosLeakageCurrent(unsigned int num_stacks_, double uni_stacked_mos_width_, unsigned int input_vector_) const
    {
        vector<double> stacked_mos_widths_(num_stacks_, uni_stacked_mos_width_);
        return calculateNmosLeakageCurrent(num_stacks_, stacked_mos_widths_, input_vector_);
    }

    //Returns the leakage current of NMOS transistors, given the transistor stakcing, transistor widths, and input combination
    double TechModel::calculateNmosLeakageCurrent(unsigned int num_stacks_, const vector<double>& stacked_mos_widths_, unsigned int input_vector_) const
    {
        // Get technology parameters
        double vdd = get("Vdd");
        double temp = get("Temperature");
        double char_temp = get("Nmos->CharacterizedTemperature");
        double min_off_current = get("Nmos->MinOffCurrent");
        double off_current = get("Nmos->OffCurrent");
        double subthreshold_swing = get("Nmos->SubthresholdSwing");
        double dibl = get("Nmos->DIBL");
        double temp_swing = get("Nmos->SubthresholdTempSwing");

        // Map dibl to a swing value for easier calculation
        double dibl_swing = subthreshold_swing / dibl;
        
        //Calculate the leakage current factor
        double leakage_current_factor = calculateLeakageCurrentFactor(num_stacks_, stacked_mos_widths_, input_vector_, vdd, subthreshold_swing, dibl_swing);

        // Calcualte actual leakage current at characterized temperature
        double leakage_current_char_tmp = stacked_mos_widths_[0] * off_current * std::pow(10.0, leakage_current_factor);
        leakage_current_char_tmp = std::max(min_off_current, leakage_current_char_tmp);

        // Calculate actual leakage current at temp
        double leakage_current = leakage_current_char_tmp * std::pow(10.0, (temp - char_temp) / temp_swing);

        return leakage_current;
    }

    double TechModel::calculatePmosLeakageCurrent(unsigned int num_stacks_, double uni_stacked_mos_width_, unsigned int input_vector_) const
    {
        vector<double> stacked_mos_widths_(num_stacks_, uni_stacked_mos_width_);
        return calculatePmosLeakageCurrent(num_stacks_, stacked_mos_widths_, input_vector_);
    }

    //Returns the leakage current of PMOS transistors, given the transistor stakcing, transistor widths, and input combination
    double TechModel::calculatePmosLeakageCurrent(unsigned int num_stacks_, const vector<double>& stacked_mos_widths_, unsigned int input_vector_) const
    {
        // Get technology parameters
        double vdd = get("Vdd");
        double temp = get("Temperature");
        double char_temp = get("Pmos->CharacterizedTemperature");
        double min_off_current = get("Pmos->MinOffCurrent");
        double off_current = get("Pmos->OffCurrent");
        double dibl = get("Pmos->DIBL");
        double subthreshold_swing = get("Pmos->SubthresholdSwing");
        double temp_swing = get("Nmos->SubthresholdTempSwing");

        // Map dibl to a swing value for easier calculation
        double dibl_swing = subthreshold_swing / dibl;
        
        //Calculate the leakage current factor
        double leakage_current_factor = calculateLeakageCurrentFactor(num_stacks_, stacked_mos_widths_, input_vector_, vdd, subthreshold_swing, dibl_swing);

        // Calcualte actual leakage current at characterized temperature
        double leakage_current_char_tmp = stacked_mos_widths_[0] * off_current * std::pow(10.0, leakage_current_factor);
        leakage_current_char_tmp = std::max(min_off_current, leakage_current_char_tmp);

        // Calculate actual leakage current at temp
        double leakage_current = leakage_current_char_tmp * std::pow(10.0, (temp - char_temp) / temp_swing);

        return leakage_current;
    }

    //Returns the leakage current, given the transistor stakcing, transistor widths, input combination,
    //and technology information (vdd, subthreshold swing, subthreshold dibl swing)
    double TechModel::calculateLeakageCurrentFactor(unsigned int num_stacks_, const vector<double>& stacked_mos_widths_, unsigned int input_vector_, double vdd_, double subthreshold_swing_, double dibl_swing_) const
    {
        // check everything is valid
        ASSERT(num_stacks_ >= 1, "[Error] Number of stacks must be >= 1!");
        ASSERT(stacked_mos_widths_.size() == num_stacks_, "[Error] Mismatch in number of stacks and the widths specified!");

        //Use short name in this method
        const double s1 = subthreshold_swing_;
        const double s2 = dibl_swing_;

        // Decode input combinations from input_vector_
        std::vector<double> vs(num_stacks_, 0.0);
        for(int i = 0; i < (int)num_stacks_; ++i)
        {
            double current_input = (double(input_vector_ & 0x1))*vdd_;
            vs[i] = (current_input);
            input_vector_ >>= 1;
        }
        // If the widths pointer is NULL, width is set to 1 by default
        vector<double> ws = stacked_mos_widths_;

        //Solve voltages at internal nodes of stacked transistors
        // v[0] = 0
        // v[num_stacks_] = vdd_
        // v[i] = (1.0/(2*s1 + s2))*((s1 + s2)*v[i - 1] + s1*v[i + 1] 
        //                 + s2*(vs[i + 1] - vs[i]) + s1*s2*log10(ws[i + 1]/ws[i]))
        //Use tri-matrix solver to solve the above linear system

        double A = -(s1 + s2);
        double B = 2*s1 + s2;
        double C = -s1;
        std::vector<double> a(num_stacks_ - 1, 0);
        std::vector<double> b(num_stacks_ - 1, 0);
        std::vector<double> c(num_stacks_ - 1, 0);
        std::vector<double> d(num_stacks_ - 1, 0);
        std::vector<double> v(num_stacks_ + 1, 0);
        unsigned int eff_num_stacks = num_stacks_;
        bool is_found_valid_v = false;
        do
        {
            //Set boundary condition
            v[0] = 0;
            v[eff_num_stacks] = vdd_;

            //If the effective number of stacks is 1, no matrix needs to be solved
            if(eff_num_stacks == 1)
            {
                break;
            }

            //----------------------------------------------------------------------
            //Setup the tri-matrix
            //----------------------------------------------------------------------
            for(int i = 0; i < (int)eff_num_stacks-2; ++i)
            {
                a[i + 1] = A;
                c[i] = C;
            }
            for(int i = 0; i < (int)eff_num_stacks-1; ++i)
            {
                b[i] = B;
                d[i] = s2*(vs[i + 1] - vs[i]) + s1*s2*std::log10(ws[i + 1]/ws[i]);
                if(i == ((int)eff_num_stacks - 2))
                {
                    d[i] -= C*vdd_;
                }
            }
            //----------------------------------------------------------------------

            //----------------------------------------------------------------------
            //Solve the tri-matrix
            //----------------------------------------------------------------------
            for(int i = 1; i < (int)eff_num_stacks-1; ++i)
            {
                double m = a[i]/b[i - 1];
                b[i] -= m*c[i - 1];
                d[i] -= m*d[i - 1];
            }

            v[eff_num_stacks - 1] = d[eff_num_stacks - 2]/b[eff_num_stacks - 2];
            for(int i = eff_num_stacks - 3; i >= 0; --i)
            {
                v[i + 1] = (d[i] - c[i]*v[i + 2])/b[i];
            }
            //----------------------------------------------------------------------

            //Check if the internal voltages are in increasing order
            is_found_valid_v = true;
            for(int i = 1; i <= (int)eff_num_stacks; ++i)
            {
                //If the ith internal voltage is not in increasing order
                //(i-1)th transistor is in triode region
                //Remove the transistors in triode region as it does not exist
                if(v[i] < v[i - 1])
                {
                    is_found_valid_v = false;
                    eff_num_stacks--;
                    vs.erase(vs.begin() + i - 1);
                    ws.erase(ws.begin() + i - 1);
                    break;
                }
            }
        } while(!is_found_valid_v);

        //Calculate the leakage current of the bottom transistor (first not in triode region)
        double vgs = vs[0] - v[0];
        double vds = v[1] - v[0];
        double leakage_current_factor = vgs/s1 + (vds - vdd_)/s2;
        //TODO - Check if the leakage current calculate for other transistors is identical

        return leakage_current_factor;
    }
    //-------------------------------------------------------------------------

	//-------------------------------------------------------------------------
    // TSV related Functions 
    //-------------------------------------------------------------------------

	// Calculates the capacitance of the bump that connects the TSV to a layer
	double TechModel::calculateBumpCapacitance( double bump_radius_, double tsv_radius_ ) const
	{
		ASSERT(bump_radius_ > tsv_radius_, "[Error], Bump Diameter must be greater than TSV Diamter");
		double wire_dielec_const = get("Wire->Global->DielectricConstant").toDouble();
		double wire_metal_thickness = get("Wire->Global->MetalThickness").toDouble();
		double dielec_thickness = get("TSV->DielectricThickness").toDouble();
		double diff = pow(bump_radius_,2) - pow(tsv_radius_ + dielec_thickness,2);

		double cap_bump = wire_dielec_const * 8.85e-12 * ((PI * diff) / wire_metal_thickness );
		return cap_bump;
	}

	// Calculates the capacitance of the silicon substrate between TSVs
	double TechModel::calculateSiliconCapacitance( double length_, double pitch_, double diameter_) const
	{
		double si_dielec_const = get("TSV->SiliconDielecConst").toDouble();
		double cap_si = si_dielec_const * 8.85e-12 * ((length_*PI) / acosh(pitch_ / diameter_));
		return cap_si;
	}

	// Calculates capacitance of the Silicon Oxide isolation layer around the copper
	double TechModel::calculateOxCapacitance(double radius_, double height_) const
	{
		double dielec_const = get("TSV->DielectricConstant").toDouble();
		double dielec_thickness = get("TSV->DielectricThickness").toDouble();
		double cap_ox = dielec_const * 8.85e-12 * ((2 * PI * height_) / log((radius_ + dielec_thickness) / radius_));
		return cap_ox;
	}

	// Calculates Resistance of the TSV taking in consideration frequency
	double TechModel::calculateTSVResistance( double radius_, double length_, double freq_ ) const
	{
		double resistivity = get("TSV->Resistivity").toDouble(); 
		double radius_sqr = radius_ * radius_; 
		double cross_section = radius_sqr * PI;
		double skin = calculateSkin(freq_);

		if ( skin > radius_) skin = radius_; // skin effect happens when skin < radius 

		double part1 = length_/cross_section;
		double part2 = length_/( PI * ( radius_sqr - std::pow(radius_ - skin, 2) ) );
		double resistance = resistivity * sqrt(std::pow(part1,2) + std::pow(part2,2));
		return resistance;
	}

	// Calculates TSV Capacitance as a funciton of the serialization silicon oxide and silicon substrate capacitance. 
	double TechModel::calculateTSVCapacitance(double ox_cap_, double si_cap_, double radius_, double height_, double freq_) const
	{
		return (ox_cap_ * si_cap_) / (ox_cap_ + si_cap_);
	}

	// skin effect of the TSV a certain frequency
	double TechModel::calculateSkin(double frequency ) const
	{
		double permeability = get("TSV->Permeability").toDouble();
		double conductivity = 1 / get("TSV->Resistivity").toDouble();
		double temp = permeability * conductivity * frequency * PI;
		return (1 / sqrt(temp));
	}
	// -----------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Wire Related Functions
    //-------------------------------------------------------------------------
    bool TechModel::isWireLayerExist(const String& layer_name_) const
    {
        std::set<String>::const_iterator it;
        it = m_available_wire_layers_->find(layer_name_);
        return (it != m_available_wire_layers_->end());
    }

    const std::set<String>* TechModel::getAvailableWireLayers() const
    {
        return m_available_wire_layers_;
    }

    double TechModel::calculateWireCapacitance(const String& layer_name_, double width_, double spacing_, double length_) const
    {
        // Get technology parameter
        double min_width = get("Wire->" + layer_name_ + "->MinWidth").toDouble();
        double min_spacing = get("Wire->" + layer_name_ + "->MinSpacing").toDouble();
        double metal_thickness = get("Wire->" + layer_name_ + "->MetalThickness").toDouble();
        double dielec_thickness = get("Wire->" + layer_name_ + "->DielectricThickness").toDouble();
        double dielec_const = get("Wire->" + layer_name_ + "->DielectricConstant").toDouble();

        ASSERT(width_ >= min_width, "[Error] Wire width must be >= " + (String) min_width + "!");
        ASSERT(spacing_ >= min_spacing, "[Error] Wire spacing must be >= " + (String) min_spacing + "!");
        ASSERT(length_ >= 0, "[Error] Wire length must be >= 0!");

        double A, B, C;
        // Calculate ground capacitance
        A = width_ / dielec_thickness;
        B = 2.04*std::pow((spacing_ / (spacing_ + 0.54 * dielec_thickness)), 1.77);
        C = std::pow((metal_thickness / (metal_thickness + 4.53 * dielec_thickness)), 0.07);
        double unit_gnd_cap = dielec_const * 8.85e-12 * (A + B * C);

        A = 1.14 * (metal_thickness / spacing_) * std::exp(-4.0 * spacing_ / (spacing_ + 8.01 * dielec_thickness));
        B = 2.37 * std::pow((width_ / (width_ + 0.31 * spacing_)), 0.28);
        C = std::pow((dielec_thickness / (dielec_thickness + 8.96 * spacing_)), 0.76) * 
                std::exp(-2.0 * spacing_ / (spacing_ + 6.0 * dielec_thickness));
        double unit_coupling_cap = dielec_const * 8.85e-12 * (A + B * C);

        double total_cap = 2 * (unit_gnd_cap + unit_coupling_cap) * length_;
        return total_cap;
    }

    double TechModel::calculateWireResistance(const String& layer_name_, double width_, double length_) const
    {
        // Get technology parameter
        double min_width = get("Wire->" + layer_name_ + "->MinWidth");
        double resistivity = get("Wire->" + layer_name_ + "->Resistivity");
        double metal_thickness = get("Wire->" + layer_name_ + "->MetalThickness");

        ASSERT(width_ >= min_width, "[Error] Wire width must be >= " + (String) min_width + "!");
        ASSERT(length_ >= 0, "[Error] Wire length must be >= 0!");

        double unit_res = resistivity / (width_ * metal_thickness);
        double total_res = unit_res * length_;

        return total_res;
    }
    //-------------------------------------------------------------------------

    TechModel::TechModel(const TechModel& tech_model_)
        : Config(tech_model_), m_std_cell_lib_(tech_model_.m_std_cell_lib_)
    {}
} // namespace DSENT
