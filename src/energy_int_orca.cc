#include "energy_int_orca.h"

energy::interfaces::orca::sysCallInterface::sysCallInterface(coords::Coordinates * cp) :
  energy::interface_base(cp), energy(0.0)
{
  if (Config::get().energy.dftb.opt > 0) optimizer = true;
  else optimizer = false;
	charge = Config::get().energy.dftb.charge;
}

energy::interfaces::orca::sysCallInterface::sysCallInterface(sysCallInterface const & rhs, coords::Coordinates *cobj) :
  interface_base(cobj), energy(rhs.energy)
{
	optimizer = rhs.optimizer;
	charge = rhs.charge;
  interface_base::operator=(rhs);
}

energy::interface_base * energy::interfaces::orca::sysCallInterface::clone(coords::Coordinates * coord_object) const
{
  sysCallInterface * tmp = new sysCallInterface(*this, coord_object);
  return tmp;
}

energy::interface_base * energy::interfaces::orca::sysCallInterface::move(coords::Coordinates * coord_object)
{
  sysCallInterface * tmp = new sysCallInterface(*this, coord_object);
  return tmp;
}

void energy::interfaces::orca::sysCallInterface::swap(interface_base &rhs)
{
  swap(dynamic_cast<sysCallInterface&>(rhs));
}

void energy::interfaces::orca::sysCallInterface::swap(sysCallInterface &rhs)
{
  interface_base::swap(rhs);
}

energy::interfaces::orca::sysCallInterface::~sysCallInterface(void) {}

/**checks if all atom coordinates are numbers*/
//bool energy::interfaces::orca::sysCallInterface::check_structure()
//{
//  bool structure = true;
//  double x, y, z;
//  for (auto i : (*this->coords).xyz())
//  {
//    x = i.x();
//    y = i.y();
//    z = i.z();
//
//    if (std::isnan(x) || std::isnan(y) || std::isnan(z))
//    {
//      std::cout << "Atom coordinates are not a number. Treating structure as broken.\n";
//      structure = false;
//      break;
//    }
//  }
//  return structure;
//}

void energy::interfaces::orca::sysCallInterface::write_inputfile(int t)
{
	std::ofstream inp;
	inp.open("orca.inp");

	inp << "! " << Config::get().energy.orca.method << " " << Config::get().energy.orca.basisset << "\n";  // method and basisset

	inp << "\n";  // empty line
	inp << "*xyz " << charge << " " << Config::get().energy.orca.multiplicity << "\n";  // headline for geometry input
	for (auto i{ 0u }; i < coords->atoms().size(); ++i)  // coordinates definition for every atom
	{
		inp << coords->atoms(i).symbol() << "  " << std::setw(12) << std::setprecision(6) << coords->xyz(i).x()
			<< "  " << std::setw(12) << std::setprecision(6) << coords->xyz(i).y()
			<< "  " << std::setw(12) << std::setprecision(6) << coords->xyz(i).z() << "\n";
	}
	inp << "*\n";  // end of coordinates definition
}

double energy::interfaces::orca::sysCallInterface::read_output(int t)
{
	std::ifstream out;
	out.open("output_orca.txt");

	std::string line;
	std::vector<std::string> linevec;
	while (out.eof() == false)
	{
		std::getline(out, line);
		if (line.substr(0, 25) == "FINAL SINGLE POINT ENERGY")
		{
			linevec = split(line, ' ', true);
			double energy_in_hartree = std::stod(linevec[4]);
			energy = energy_in_hartree * energy::au2kcal_mol;

			std::cout << "energy in hartree: " << energy_in_hartree << "\n";
		}
	}
  return energy;
}

/*
Energy class functions that need to be overloaded
*/

// Energy function
double energy::interfaces::orca::sysCallInterface::e(void)
{
  //integrity = check_structure();
  /*if (integrity == true)
  {*/
    write_inputfile(0);
    scon::system_call(Config::get().energy.orca.path + " orca.inp > output_orca.txt");
    energy = read_output(0);
    return energy;
  //}
  //else return 0;  // energy = 0 if structure contains NaN
}

// Energy+Gradient function
double energy::interfaces::orca::sysCallInterface::g(void)
{
  //integrity = check_structure();
  //if (integrity == true)
  //{
    write_inputfile(1);
    scon::system_call(Config::get().energy.orca.path + " orca_input.txt > output_orca.txt");
    energy = read_output(1);
    return energy;
  //}
  //else return 0;  // energy = 0 if structure contains NaN
}

// Hessian function
double energy::interfaces::orca::sysCallInterface::h(void)
{
	return 0;
  //integrity = check_structure();
  //if (integrity == true)
  //{
    //write_inputfile(2);
    //scon::system_call(Config::get().energy.dftb.path + " > output_dftb.txt");
    //energy = read_output(2);
    //return energy;
  //}
  //else return 0;  // energy = 0 if structure contains NaN
}

// Optimization
double energy::interfaces::orca::sysCallInterface::o(void)
{
  //integrity = check_structure();
  //if (integrity == true)
  //{
    write_inputfile(3);
    scon::system_call(Config::get().energy.orca.path + " orca_input.txt > output_dftb.txt");
    energy = read_output(3);
    return energy;
  //}
  //else return 0;  // energy = 0 if structure contains NaN
}

// Output functions
void energy::interfaces::orca::sysCallInterface::print_E(std::ostream &S) const
{
  S << "Total Energy:      ";
  S << std::right << std::setw(16) << std::fixed << std::setprecision(8) << energy;
}

void energy::interfaces::orca::sysCallInterface::print_E_head(std::ostream &S, bool const endline) const
{
  S << "Energies\n";
  S << std::right << std::setw(24) << "E_bs";
  S << std::right << std::setw(24) << "E_coul";
  S << std::right << std::setw(24) << "E_rep";
  S << std::right << std::setw(24) << "SUM\n";
  if (endline) S << '\n';
}

void energy::interfaces::orca::sysCallInterface::print_E_short(std::ostream &S, bool const endline) const
{
  S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << 0;
  S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << 0;
  S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << 0;
  S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << energy << '\n';
  if (endline) S << '\n';
}

void energy::interfaces::orca::sysCallInterface::to_stream(std::ostream&) const { }

//double energy::interfaces::orca::sysCallInterface::calc_self_interaction_of_external_charges()
//{
//  double energy{ 0.0 };
//  for (auto i = 0u; i < Config::get().energy.qmmm.mm_charges.size(); ++i)
//  {
//    auto c1 = Config::get().energy.qmmm.mm_charges[i];
//    for (auto j = 0u; j < i; ++j)
//    {
//      auto c2 = Config::get().energy.qmmm.mm_charges[j];
//
//      double dist_x = c1.x - c2.x;
//      double dist_y = c1.y - c2.y;
//      double dist_z = c1.z - c2.z;
//      double dist = std::sqrt(dist_x*dist_x + dist_y * dist_y + dist_z * dist_z);  // distance in angstrom
//
//      energy += 332.0 * c1.charge * c2.charge / dist;  // energy in kcal/mol
//    }
//  }
//  return energy;
//}

bool energy::interfaces::orca::sysCallInterface::check_bond_preservation(void) const
{
  std::size_t const N(coords->size());
  for (std::size_t i(0U); i < N; ++i)
  { // cycle over all atoms i
    if (!coords->atoms(i).bonds().empty())
    {
      std::size_t const M(coords->atoms(i).bonds().size());
      for (std::size_t j(0U); j < M && coords->atoms(i).bonds(j) < i; ++j)
      { // cycle over all atoms bound to i
        double const L(geometric_length(coords->xyz(i) - coords->xyz(coords->atoms(i).bonds(j))));
        double const max = 1.2 * (coords->atoms(i).cov_radius() + coords->atoms(coords->atoms(i).bonds(j)).cov_radius());
        if (L > max) return false;
      }
    }
  }
  return true;
}

bool energy::interfaces::orca::sysCallInterface::check_atom_dist(void) const
{
  std::size_t const N(coords->size());
  for (std::size_t i(0U); i < N; ++i)
  {
    for (std::size_t j(0U); j < i; j++)
    {
      if (dist(coords->xyz(i), coords->xyz(j)) < 0.3)
      {
        return false;
      }
    }
  }
  return true;
}

std::vector<coords::float_type>
energy::interfaces::orca::sysCallInterface::charges() const
{
	std::vector<coords::float_type> charges;
	return charges;
}

std::vector<coords::Cartesian_Point>
energy::interfaces::orca::sysCallInterface::get_g_ext_chg() const
{
  return grad_ext_charges;
}

