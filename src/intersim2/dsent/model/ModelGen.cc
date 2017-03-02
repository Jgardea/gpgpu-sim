#include "model/ModelGen.h"

#include <iostream>

#include "model/Model.h"
// Standard cells
#include "std_cells/StdCell.h"
#include "std_cells/INV.h"
#include "std_cells/NAND2.h"
#include "std_cells/NOR2.h"
#include "std_cells/MUX2.h"
#include "std_cells/XOR2.h"
#include "std_cells/DFFQ.h"
#include "std_cells/LATQ.h"
#include "std_cells/ADDF.h"
#include "std_cells/OR2.h"
#include "std_cells/AND2.h"
#include "std_cells/BUF.h"
// Electrical functional units
#include "electrical/TestModel.h"
#include "electrical/RippleAdder.h"
#include "electrical/Multiplexer.h"
#include "electrical/MultiplexerCrossbar.h"
#include "electrical/OR.h"
#include "electrical/Decoder.h"
#include "electrical/DFFRAM.h"
#include "electrical/MatrixArbiter.h"
#include "electrical/SeparableAllocator.h"
#include "electrical/Router.h"
#include "electrical/RepeatedLink.h"
#include "electrical/BroadcastHTree.h"
// Networks
#include "model/BSElectricalMesh.h"           //jgardea
#include "model/ElectricalMesh.h"

namespace DSENT
{
    using std::cout;
    using std::endl;

    //TODO: Eventually automate the creation of this file

    Model* ModelGen::createModel(const String& model_name_, const String& instance_name_, const TechModel* tech_model_)
    {
        Log::printLine("ModelGen::createModel -> " + model_name_);
        
        if("INV" == model_name_)
        {
            return new INV(instance_name_, tech_model_);
        }
        else if("NAND2" == model_name_)
        {
            return new NAND2(instance_name_, tech_model_);
        }
        else if("NOR2" == model_name_)
        {
            return new NOR2(instance_name_, tech_model_);
        }
        else if("MUX2" == model_name_)
        {
            return new MUX2(instance_name_, tech_model_);
        }
        else if("XOR2" == model_name_)
        {
            return new XOR2(instance_name_, tech_model_);
        }
        else if("DFFQ" == model_name_)
        {
            return new DFFQ(instance_name_, tech_model_);
        }
        else if("LATQ" == model_name_)
        {
            return new LATQ(instance_name_, tech_model_);
        }
        else if("ADDF" == model_name_)
        {
            return new ADDF(instance_name_, tech_model_);
        }
        else if("OR2" == model_name_)
        {
            return new OR2(instance_name_, tech_model_);
        }
        else if("AND2" == model_name_)
        {
            return new AND2(instance_name_, tech_model_);
        }
        else if("BUF" == model_name_)
        {
            return new BUF(instance_name_, tech_model_);
        }
        else if("TestModel" == model_name_)
        {
            return new TestModel(instance_name_, tech_model_);
        }
        else if("RippleAdder" == model_name_)
        {
            return new RippleAdder(instance_name_, tech_model_);
        }
        else if("Multiplexer" == model_name_)
        {
            return new Multiplexer(instance_name_, tech_model_);
        }
        else if("OR" == model_name_)
        {
            return new OR(instance_name_, tech_model_);
        }
        else if("MultiplexerCrossbar" == model_name_)
        {
            return new MultiplexerCrossbar(instance_name_, tech_model_);
        }
        else if("Decoder" == model_name_)
        {
            return new Decoder(instance_name_, tech_model_);
        }
        else if("DFFRAM" == model_name_)
        {
            return new DFFRAM(instance_name_, tech_model_);
        }
        else if("MatrixArbiter" == model_name_)
        {
            return new MatrixArbiter(instance_name_, tech_model_);
        }
        else if("SeparableAllocator" == model_name_)
        {
            return new SeparableAllocator(instance_name_, tech_model_);
        }
        else if("Router" == model_name_)
        {
            return new Router(instance_name_, tech_model_);
        }
        else if("RepeatedLink" == model_name_)
        {
            return new RepeatedLink(instance_name_, tech_model_);
        }
        else if("BroadcastHTree" == model_name_)
        {
            return new BroadcastHTree(instance_name_, tech_model_);
        }
        else if("ElectricalMesh" == model_name_)
        {
            return new ElectricalMesh(instance_name_, tech_model_);
        }
        else if("BSElectricalMesh" == model_name_ )                  // jgardea
        {
            return new BSElectricalMesh(instance_name_, tech_model_);
        }
        else
        {
            const String& error_msg = "[Error] Invalid model name (" + model_name_ + ")";
            throw Exception(error_msg);
            return NULL;
        }
        return NULL;
    }
    
    StdCell* ModelGen::createStdCell(const String& std_cell_name_, const String& instance_name_, const TechModel* tech_model_)
    {
        Log::printLine("ModelGen::createStdCell -> " + std_cell_name_);
        
        if("INV" == std_cell_name_)
        {
            return new INV(instance_name_, tech_model_);
        }
        else if("NAND2" == std_cell_name_)
        {
            return new NAND2(instance_name_, tech_model_);
        }
        else if("NOR2" == std_cell_name_)
        {
            return new NOR2(instance_name_, tech_model_);
        }
        else if("MUX2" == std_cell_name_)
        {
            return new MUX2(instance_name_, tech_model_);
        }
        else if("XOR2" == std_cell_name_)
        {
            return new XOR2(instance_name_, tech_model_);
        }
        else if("DFFQ" == std_cell_name_)
        {
            return new DFFQ(instance_name_, tech_model_);
        }
        else if("LATQ" == std_cell_name_)
        {
            return new LATQ(instance_name_, tech_model_);
        }
        else if("ADDF" == std_cell_name_)
        {
            return new ADDF(instance_name_, tech_model_);
        }
        else if("OR2" == std_cell_name_)
        {
            return new OR2(instance_name_, tech_model_);
        }
        else if("AND2" == std_cell_name_)
        {
            return new AND2(instance_name_, tech_model_);
        }
        else if("BUF" == std_cell_name_)
        {
            return new BUF(instance_name_, tech_model_);
        }
        else
        {
            const String& error_msg = "[Error] Invalid Standard Cell name (" + std_cell_name_ + ")";
            throw Exception(error_msg);
            return NULL;
        }
        return NULL;
    }

    ElectricalModel* ModelGen::createRAM(const String& ram_name_, const String& instance_name_, const TechModel* tech_model_)
    {
        Log::printLine("ModelGen::createRAM -> " + ram_name_);
        
        if("DFFRAM" == ram_name_)
        {
            return new DFFRAM(instance_name_, tech_model_);
        }
        else
        {
            const String& error_msg = "[Error] Invalid RAM name (" + ram_name_ + ")";
            throw Exception(error_msg);
            return NULL;
        }
        return NULL;
    }

    ElectricalModel* ModelGen::createCrossbar(const String& crossbar_name_, const String& instance_name_, const TechModel* tech_model_)
    {
        Log::printLine("ModelGen::createCrossbar -> " + crossbar_name_);
        
        if("MultiplexerCrossbar" == crossbar_name_)
        {
            return new MultiplexerCrossbar(instance_name_, tech_model_);
        }
        else
        {
            const String& error_msg = "[Error] Invalid Crossbar name (" + crossbar_name_ + ")";
            throw Exception(error_msg);
            return NULL;
        }
        return NULL;
    }
    //-----------------------------------------------------------------

    void ModelGen::printAvailableModels()
    {
        // TODO: Need more descriptions
        cout << "INV NAND2 NOR2 MUX2 XOR2 DFFQ LATQ ADDF OR2 AND2 BUF" << endl;
        cout << "RippleAdder Multiplexer OR RepeatedLink BroadcastHTree" << endl;
        cout << "MultiplexerCrossbar Decoder DFFRAM MatrixArbiter SeparableAllocator Router" << endl;
        cout << "ElectricalMesh ElectricalClos PhotonicClos" << endl;
        return;
    }
} // namespace DSENT

