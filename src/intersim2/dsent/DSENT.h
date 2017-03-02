#ifndef __DSENT_DSENT_H__
#define __DSENT_DSENT_H__

#include <vector>
#include <utility>
#include <string>

// For DSENT operations
#include "libutil/OptionParser.h"
#include "libutil/Calculator.h"
#include "util/CommonType.h"
#include "util/ConfigUtil.h"
#include "util/Result.h"
#include "model/Model.h"
#include "model/ModelGen.h"

// For timing optimization
#include "model/ElectricalModel.h"
#include "timing_graph/ElectricalNet.h"
#include "timing_graph/ElectricalTimingTree.h"
#include "timing_graph/ElectricalTimingOptimizer.h"
#include "model/PortInfo.h"

namespace DSENT
{
    using LibUtil::OptionParser;
    using LibUtil::Calculator;
    using std::string;

    class DSENT
    {
        protected:
            class DSENTCalculator : public Calculator
            {
                public:
                    DSENTCalculator();
                    virtual ~DSENTCalculator();
        
                protected:
                    virtual double getEnvVar(const String& var_name_) const;
            }; // class DSENTCalculator

        public:
            //static void run(int argc_, char** argv_);
            static vector< pair<String, vector<pair<String, double> > > > run(vector<string>);
            static double findResult( string component, string event);

        protected:
            static void setRuntimeOptions(OptionParser* option_parser_);
            //static void initialize(int argc_, char** argv_);
            static void initialize(vector<string>);
            static void buildModel();
            static void processQuery();
            static const void* processQuery(const String& query_str_, bool is_print_);
            static void finalize();

            static void performTimingOpt();
            static void reportTiming();

            static void processEvaluate();

            // jgardea
            static void getResults( );
            static int findComponent( String comp );
            static void printResults( );

        protected:
            static Model* ms_model_;

            static bool ms_is_verbose_;
        
            static bool is_asymmetric;          // jgardea
            static vector< const Result*> ms_results;
            static vector< pair<String, vector<pair<String, double> > > > ms_energy_results; // jgardea


    }; // class DSENT

} // namespace DSENT

#endif // __DSENT_DSENT_H__

