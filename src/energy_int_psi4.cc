#include "energy_int_psi4.h"

void energy::interfaces::psi4::sysCallInterface::swap(interface_base & other){
  auto casted = dynamic_cast<sysCallInterface*>(&other);
  if(!casted) {
    throw std::runtime_error("Failed to dynamically cast the energy interface in 'energy::interfaces::psi4::sysCallInterface::swap'!\n"
    "to be a sysCallInterface.\n");
  }
  interface_base::swap(*casted);
}

energy::interface_base * energy::interfaces::psi4::sysCallInterface::clone(coords::Coordinates* coord_object) const{ return new sysCallInterface(*this, coord_object); }
energy::interface_base * energy::interfaces::psi4::sysCallInterface::move(coords::Coordinates* coord_object) { return new sysCallInterface(*this, coord_object); }

//void energy::interfaces::psi4::sysCallInterface::update(bool const skip_topology){}

coords::float_type energy::interfaces::psi4::sysCallInterface::e(void){
  write_input();
  make_call();
  return parse_energy();
}
coords::float_type energy::interfaces::psi4::sysCallInterface::g(void){
  write_input(Calc::gradient);
  make_call();
  auto e_grads = parse_gradients();
  coords->set_g_xyz(std::move(e_grads.second));
  return e_grads.first;
}
coords::float_type energy::interfaces::psi4::sysCallInterface::h(void){
  return 0.0;
}
coords::float_type energy::interfaces::psi4::sysCallInterface::o(void){
  write_input(Calc::optimize);
  make_call();
  auto e_geo_grads = parse_geometry_and_gradients();
  coords->set_xyz(std::move(std::get<1>(e_geo_grads)));
  coords->set_g_xyz(std::move(std::get<2>(e_geo_grads)));
  return std::get<0>(e_geo_grads);
}

void energy::interfaces::psi4::sysCallInterface::print_E(std::ostream& os) const{
  for(auto&& pair:energies){
    os << std::right << pair.first << ": ";
    os << std::left << std::setw(16) << std::fixed << std::setprecision(8) << pair.second*energy::au2kcal_mol;
    os << "\n";
  }
}

void energy::interfaces::psi4::sysCallInterface::print_E_head(std::ostream& os, bool const endline) const{
  for(auto&& pair: energies){
    os << std::right << std::setw(26) << pair.first;
  }
  if(endline) os << "\n";
}

void energy::interfaces::psi4::sysCallInterface::print_E_short(std::ostream& os, bool const endline) const{
  for(auto&& pair: energies){
    os << std::right << std::setw(26) << std::scientific << std::setprecision(5) << pair.second*energy::au2kcal_mol;
  }
  if(endline) os << "\n";
}

void energy::interfaces::psi4::sysCallInterface::to_stream(std::ostream&) const{}

void energy::interfaces::psi4::sysCallInterface::write_input(energy::interfaces::psi4::sysCallInterface::Calc kind) const{
  std::ofstream ofs(tmp_file_name + "_inp.dat");
  if(kind == Calc::energy){
    write_energy_input(ofs);
  }
  else if(kind==Calc::gradient){
    write_gradients_input(ofs);
  }
  else if(kind==Calc::optimize){
    write_optimize_input(ofs);
  }
  else{
    throw std::runtime_error("Selfdestruction initiated. Something went terribly wrong, call a developer!");
  }
}
void energy::interfaces::psi4::sysCallInterface::write_head(std::ostream& os) const{
  auto const& memory = Config::get().energy.psi4.memory;
  auto const& basis = Config::get().energy.psi4.basis;
  os << "memory " << memory << "\n"
    "set basis " << basis << "\n\n";
  write_molecule(os);
}

void energy::interfaces::psi4::sysCallInterface::write_molecule(std::ostream& os) const{
  auto const& spin = Config::get().energy.psi4.spin;
  auto const& charge = Config::get().energy.psi4.charge;
  os << "molecule mol{\n"
    "  " << charge << " " << spin << "\n"
    << coords::output::formats::xyz(*coords)
    << "}\n\n";
}

void energy::interfaces::psi4::sysCallInterface::write_energy_input(std::ostream& os) const{
  auto const& method = Config::get().energy.psi4.method;
  write_head(os);
  os << "energy ('" << method << "')";
}
void energy::interfaces::psi4::sysCallInterface::write_gradients_input(std::ostream& os) const{
  auto const& method = Config::get().energy.psi4.method;
  write_head(os);
  os << "gradient ('" << method << "')";
}
void energy::interfaces::psi4::sysCallInterface::write_optimize_input(std::ostream& os) const{
  auto const& method = Config::get().energy.psi4.method;
  write_head(os);
  os << "optimize ('" << method << "')";
}

void energy::interfaces::psi4::sysCallInterface::make_call()const{
  std::stringstream call_stream;
  auto const& path = Config::get().energy.psi4.path;
	call_stream << path << " "
    << tmp_file_name << "_inp.dat "
    << tmp_file_name << "_out.dat";

  auto failcount = 0u;
  for (; failcount < 3u; ++failcount) {
		auto ret = scon::system_call(call_stream.str());
		if (ret == 0) {
			break;
		}
		else {
			std::cout << "I am failing to call Psi4! Are you sure you passed the right Psi4 path?\n";
			std::cout << "The path passed is: \"" << path << "\"\n";
		}
	}
	if (failcount == 3) {
		throw std::runtime_error("3 Psi4 calls failed!");
	}
}

std::vector<std::string> energy::interfaces::psi4::sysCallInterface::parse_specific_position(std::istream& is, std::string const& delim, int space) const{
  std::vector<std::string> mol;
  for(std::string line; getline(is,line);){
    if(line.find(delim, 0) != std::string::npos){
      for(auto i = 0; i < space; ++i) getline(is,line);
      do{
        if(line.empty()){
          break;
        }
        mol.emplace_back(line);
      }while(getline(is,line));
      break;
    }
  }
  return mol;
}

coords::Representation_3D energy::interfaces::psi4::sysCallInterface::get_final_geometry() const{
  std::ifstream ifs(tmp_file_name + "_out.dat");
  return extract_Rep3D(parse_specific_position(ifs, "Final optimized geometry", 6));
}

std::vector<std::string> energy::interfaces::psi4::sysCallInterface::get_last_gradients()const{
  std::ifstream ifs(tmp_file_name + "_out.dat");
  std::vector<std::string> grads;
  for(auto tmp_grads = parse_specific_position(ifs, "Total Grad", 3);
    !tmp_grads.empty(); tmp_grads = parse_specific_position(ifs, "Total Grad", 3)){
    grads = tmp_grads;
  }
  return grads;
}

coords::float_type energy::interfaces::psi4::sysCallInterface::parse_energy(){
  std::ifstream ifs(tmp_file_name + "_out.dat");
  energies.clear();
  std::vector<std::string> energy;

  for(auto tmp_energy = parse_specific_position(ifs, "Nuclear Repulsion Energy", 0);
    !tmp_energy.empty(); tmp_energy = parse_specific_position(ifs, "Nuclear Repulsion Energy", 0)){
    energy = tmp_energy;
  }
  for(auto & line : energy){
    std::istringstream iss{line};
    std::vector<std::string> words{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    auto val = std::stod(words.back());
    std::string key = words.front();
    std::for_each(words.cbegin()+1, words.cend()-2, [&key](auto&& word){
      key += " " + word;
    });
    energies.emplace_back(std::make_pair(key, val));
  }
  return energies.back().second*energy::au2kcal_mol;
}

std::pair<coords::float_type, coords::Representation_3D> energy::interfaces::psi4::sysCallInterface::parse_gradients(){
  auto energy = parse_energy();
  auto grads = extract_Rep3D(get_last_gradients());
  for (auto && g: grads){
    g *= energy::Hartree_Bohr2Kcal_MolAng;
  }
  return std::make_pair(energy, grads);
}

std::tuple<coords::float_type, coords::Representation_3D, coords::Representation_3D>
energy::interfaces::psi4::sysCallInterface::parse_geometry_and_gradients(){
  auto geo = get_final_geometry();
  auto energy_and_geo = parse_gradients();
  return std::make_tuple(energy_and_geo.first, geo, energy_and_geo.second);
}