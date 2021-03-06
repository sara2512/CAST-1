/**
CAST 3
energy_int_3layer.h
Purpose: three-layer-interface (analogous to subtractive QMMM)

available interfaces: OPLSAA, AMBER, MOPAC, DFTB+, PSI4, GAUSSIAN, ORCA


@author Susanne Sauer
@version 1.0
*/

#pragma once
#include "coords.h"
#include "coords_io.h"
#include <vector>
#include "coords_atoms.h"
#include "energy_int_aco.h"
#include "energy_int_mopac.h"
#include "tinker_parameters.h"
#include "helperfunctions.h"
#include "modify_sk.h"
#include "qmmm_helperfunctions.h"

namespace energy
{
  namespace interfaces
  {
    /**namespace for QMMM interfaces*/
    namespace qmmm
    {
      /**Three-Layer interface class*/
      class THREE_LAYER
        : public interface_base
      {

        /**uncontracted forcefield parameters*/
        static ::tinker::parameter::parameters tp;

      public:

        /**Constructor*/
        THREE_LAYER(coords::Coordinates*);
        /**overloaded Constructor*/
        THREE_LAYER(THREE_LAYER const&, coords::Coordinates*);
        /**another overload of Constructor*/
        THREE_LAYER(THREE_LAYER&&, coords::Coordinates*);

        /*
        Energy class functions that need to be overloaded (for documentation see also energy.h)
        */

        interface_base* clone(coords::Coordinates*) const;
        interface_base* move(coords::Coordinates*);

        void swap(interface_base&);
        void swap(THREE_LAYER&);

        /** update structure (account for topology or rep change)*/
        void update(bool const skip_topology = false);

        /**sets the atom coordinates of the subsystems (QM and MM) to those of the whole coordobject*/
        void update_representation();

        /** Energy function*/
        coords::float_type e() override;
        /** Energy+Gradient function */
        coords::float_type g() override;
        /** Energy+Hessian function (not existent for this interface)*/
        coords::float_type h() override;
        /** Optimization in the interface or interfaced program (not existent for this interface)*/
        coords::float_type o() override;

        /** Return charges (for QM, SE and MM atoms) */
        std::vector<coords::float_type> charges() const override;
        /**overwritten function, should not be called*/
        coords::Gradients_3D get_g_ext_chg() const override {
          throw std::runtime_error("function not implemented\n");
        }

        /**prints total energy (not implemented)*/
        void print_E(std::ostream&) const  final override;
        /**prints 'headline' for energies*/
        void print_E_head(std::ostream&, bool const endline = true) const  final override;
        /**prints partial energies*/
        void print_E_short(std::ostream&, bool const endline = true) const  final override;
        /**function not implemented*/
        void to_stream(std::ostream&) const final override {};

      private:

        /**do initialization that doesn't need to be done in constructor
        it doesn't make sense to do it in constructor as the constructor is first called without coordinates*/
        void initialization();

        /**calculates energies and gradients
        @param if_gradient: true if gradients should be calculated, false if not*/
        coords::float_type qmmm_calc(bool if_gradient);

        /**fix all QM and SE atoms + M1 atoms
        @coordobj: coordinates object where atoms should be fixed*/
        void fix_qmse_atoms(coords::Coordinates& coordobj);

        /**fix all MM atoms, except M1 atoms
        @coordobj: coordinates object where atoms should be fixed*/
        void fix_mm_atoms(coords::Coordinates& coordobj);

        /**indizes of QM atoms*/
        std::vector<size_t> qm_indices;
        /**indizes of QM + SE atoms*/
        std::vector<size_t> qmse_indices;
        /**vector of length total number of atoms
        only those elements are filled whose position corresponds to QM atoms
        they are filled with successive numbers starting from 0
        purpose: faciliate mapping between total coordinates object and subsystems*/
        std::vector<size_t> new_indices_qm;
        /**vector of length total number of atoms
        only those elements are filled whose position corresponds to atoms of the medium system (i.e. QM + SE)
        they are filled with successive numbers starting from 0
        purpose: faciliate mapping between total coordinates object and subsystems*/
        std::vector<size_t> new_indices_qmse;

        /**vector with link atoms for small system*/
        std::vector<LinkAtom> link_atoms_small;
        /**vector with link atoms for medium system*/
        std::vector<LinkAtom> link_atoms_medium;

        /**coordinates object for QM part*/
        coords::Coordinates qmc_small;
        /**SE coordinates object for QM part*/
        coords::Coordinates sec_small;
        /**SE coordinates object for medium system*/
        coords::Coordinates sec_medium;
        /**MM coordinates object for medium system*/
        coords::Coordinates mmc_medium;
        /**coordinates object for whole system*/
        coords::Coordinates mmc_big;

        /**atom index that determines center of medium region*/
        std::size_t index_of_medium_center;
        /**atom index that determines center of small region*/
        std::size_t index_of_small_center;

        /**energy of QM system*/
        coords::float_type qm_energy_small;
        /**energy of small SE system*/
        coords::float_type se_energy_small;
        /**energy of medium SE system*/
        coords::float_type se_energy_medium;
        /**energy of medium MM system*/
        coords::float_type mm_energy_medium;
        /**energy of big MM system*/
        coords::float_type mm_energy_big;
      };
    }
  }
}
