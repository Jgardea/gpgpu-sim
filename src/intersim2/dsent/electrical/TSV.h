#ifndef __DSENT_MODEL_TSV_H__
#define __DSENT_MODEL_TSV_H__

#include "util/CommonType.h"
//#include "model/ElectricalModel.h"
#include "electrical/RepeatedLink.h"

namespace DSENT
{
    class StdCell;
    class ElectricalLoad;
    class ElectricalTimingTree;

    //class TSV : public ElectricalModel
	class TSV : public RepeatedLink
    {
        public:
            TSV(const String& instance_name_, const TechModel* tech_model_); 
            virtual ~TSV();

        public:
            // Set a list of properties' name needed to construct model
            void initParameters();
            // Set a list of properties' name needed to construct model
            void initProperties();

            // Clone and return a new instance
            virtual TSV* clone() const;

        protected:
            // Build the model
            virtual void constructModel();
            virtual void updateModel();
            virtual void useModel();
            virtual void propagateTransitionInfo();

        private:
            // Use a repeater and a load to mimic a segment of the repeated link
            StdCell* m_repeater_;
            ElectricalLoad* m_repeater_load_;
            ElectricalTimingTree* m_timing_tree_;
    }; // class TSV
} // namespace DSENT

#endif // __DSENT_MODEL_TSV_H__

