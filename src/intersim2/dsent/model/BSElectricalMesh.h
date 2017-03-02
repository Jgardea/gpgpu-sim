#ifndef __DSENT_MODEL_NETWORK_BSELECTRICAL_MESH_H__
#define __DSENT_MODEL_NETWORK_BSELECTRICAL_MESH_H__

#include "util/CommonType.h"
#include "model/ElectricalModel.h"

namespace DSENT
{
    /** 
     * \brief An electrical mesh network
     */
    class BSElectricalMesh : public ElectricalModel
    {
        public:
            BSElectricalMesh(const String& instance_name_, const TechModel* tech_model_);
            virtual ~BSElectricalMesh();

        public:
            // Set a list of properties' name needed to construct model
            void initParameters();
            // Set a list of properties' name needed to construct model
            void initProperties();
            // Let DSENT know if there are asymmetric components

        protected:
            // Build the model
            virtual void constructModel();
            virtual void updateModel();
            virtual void propagateTransitionInfo();

        protected:
            //bool isAsymmetric;
            bool hasVerticalBus;
        
    }; // class BSElectricalMesh
} // namespace DSENT

#endif // __DSENT_MODEL_NETWORK_ELECTRICAL_MESH_H__

