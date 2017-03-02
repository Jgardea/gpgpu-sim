#ifndef __DSENT_MODEL_NETWORK_ELECTRICAL_MESH_H__
#define __DSENT_MODEL_NETWORK_ELECTRICAL_MESH_H__

#include "util/CommonType.h"
#include "model/ElectricalModel.h"

namespace DSENT
{
    /** 
     * \brief An electrical mesh network
     */
    class ElectricalMesh : public ElectricalModel
    {
        public:
            ElectricalMesh(const String& instance_name_, const TechModel* tech_model_);
            virtual ~ElectricalMesh();

        public:
            // Set a list of properties' name needed to construct model
            void initParameters();
            // Set a list of properties' name needed to construct model
            void initProperties();

            // Clone and return a new instance
            virtual ElectricalMesh* clone() const;

        protected:


            unsigned int _number_sites;
            unsigned int _number_bits_per_flit;
            unsigned int _number_sites_per_router;
            
            // router configuration
            
            unsigned int _rr_ports;
            unsigned int _router_number_vns;
            unsigned int _router_number_vcs_per_vn;
            unsigned int _router_number_bufs_per_vc;



            // Build the model
            virtual void constructModel();
            virtual void updateModel();
            virtual void propagateTransitionInfo();
        
    }; // class ElectricalMesh
} // namespace DSENT

#endif // __DSENT_MODEL_NETWORK_ELECTRICAL_MESH_H__

