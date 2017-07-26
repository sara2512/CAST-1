#include "energy_int_chemshell.h"

template<typename T, typename U>
auto zip(T && a, U && b) {
	std::vector<decltype(std::make_pair(*a.begin(), *b.begin()))> ret_vec;

	std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(ret_vec),
		[](auto && a_i, auto && b_i) {
		return std::make_pair(a_i, b_i);
	});
	return std::move(ret_vec);
}

void energy::interfaces::chemshell::sysCallInterface::create_pdb() const {
	
	write_xyz(tmp_file_name + ".xyz");

	std::stringstream ss;
	ss << Config::get().energy.chemshell.babel_path << " -ixyz " << tmp_file_name << ".xyz -opdb " << tmp_file_name << ".pdb";

	auto ret = scon::system_call(ss.str());

	if (ret) {
		throw std::runtime_error("Failed to call babel!");
	}

}

void energy::interfaces::chemshell::sysCallInterface::write_xyz(std::string const & o_file) const {
	std::ofstream xyz_file(o_file);
	xyz_file << coords->size() << "\n\n";
	xyz_file << coords::output::formats::xyz(*coords);
	xyz_file.close();
}

void energy::interfaces::chemshell::sysCallInterface::write_input(bool single_point) const {
	
	call_tleap();
	write_chemshell_file(single_point);
}

void energy::interfaces::chemshell::sysCallInterface::call_tleap()const {
	
	make_tleap_input(tmp_file_name);
	std::stringstream ss;

	ss << "tleap -s -f " << tmp_file_name << ".in > " << tmp_file_name << ".out";

	scon::system_call(ss.str());

}

void energy::interfaces::chemshell::sysCallInterface::make_tleap_input(std::string const & o_file)const {

	std::stringstream ss;

	ss << "antechamber -i " << o_file << ".pdb -fi pdb -o " << o_file << ".mol2 -fo mol2";

	auto ret = scon::system_call(ss.str());

	if (ret) {
		throw std::runtime_error("Failed to call antechamber!");
	}

	// To empty ss
	std::stringstream().swap(ss);

	ss << "parmchk -i " << o_file << ".mol2 -f mol2 -o " << o_file << ".frcmod";

	ret = scon::system_call(ss.str());

	if (ret) {
		throw std::runtime_error("Failed to call parmchk!");
	}

	std::stringstream().swap(ss);

	std::ofstream tleap_input(o_file + ".in");

	tleap_input <<
		"source leaprc.gaff\n"
		"LIG = loadmol2 " << o_file << ".mol2\n"
		"check LIG\n"
		"saveof LIG " << o_file << ".lib\n"
		"saveamberparm LIG " << o_file << ".prmtop " << o_file << ".inpcrd\n"
		"savepdb LIG " << o_file << ".pdb\n"
		"quit"
		;

	tleap_input.close();

}

void energy::interfaces::chemshell::sysCallInterface::make_sp_inp(std::ofstream & ofs) const {

	constexpr auto mxlist = 45000;
	constexpr auto cutoff = 1000;

	ofs << "eandg coords = ${dir}/${sys_name_id}.c \\\n"
		"    theory=hybrid : [ list \\\n"
		"        coupling= $embedding_scheme \\\n"
		"        qm_theory= $qm_theory : [ list hamiltonian = $qm_ham \\\n"
		"            restart = no \\\n"
		"            excited = no \\\n"
		"            basis = $qm_basis \\\n"
		"            eroots = 4 ] \\\n"
		"    qm_region = $qm_atoms \\\n"
		"    debug=no \\\n"
		"    mm_theory= dl_poly : [ list \\\n"
		"        list_option=none \\\n"
		"        conn= ${sys_name_id}.c \\\n"
		"        mm_defs=$amber_prmtop \\\n"
		"        exact_srf=yes \\\n"
		"        mxlist=" << mxlist << " \\\n"
		"        cutoff=" << cutoff << " \\\n"
		"        scale14 = {1.2 2.0}\\\n"
		"        amber_prmtop_file=$amber_prmtop ] ] \\\n"
		"    energy=energy.energy\\\n"
		"    gradient=energy.gradient\n"
		"\n"
		"\n"
		"close $control_input_settings\n";
}

void energy::interfaces::chemshell::sysCallInterface::make_opt_inp(std::ofstream & ofs) const {

	constexpr auto maxcycle = 1000;
	constexpr auto maxcyc = 2000;
	constexpr auto tolerance = 0.00045;
	constexpr auto mxlist = 45000;
	constexpr auto cutoff = 1000;

	std::string active_atoms = find_active_atoms();

	ofs << "dl-find coords = ${dir}/${sys_name_id}.c \\\n"
		"    coordinates=hdlc \\\n"
		"    maxcycle=" << maxcycle << " \\\n"
		"    tolerance=" << tolerance << " \\\n"
		"    active_atoms= { " << active_atoms << "} \\\n"
		"    residues= $residues \\\n"
		"    theory=hybrid : [ list \\\n"
		"        coupling= $embedding_scheme \\\n"
		"        qm_theory= $qm_theory : [ list hamiltonian = $qm_ham \\\n"
		"            basis= $qm_basis \\\n"
		"            maxcyc= " << maxcyc << " \\\n"
		"            dispersion_correction= $qm_ham \\\n"
		"            charge= $qm_ch ] \\\n"
		"    qm_region = $qm_atoms \\\n"
		"    debug=no \\\n"
		"    mm_theory= dl_poly : [ list \\\n"
		"        list_option=none \\\n"
		"        conn= ${sys_name_id}.c \\\n"
		"        mm_defs=$amber_prmtop \\\n"
		"        exact_srf=yes \\\n"
		"        mxlist=" << mxlist << " \\\n"
		"        cutoff=" << cutoff << " \\\n"
		"        scale14 = {1.2 2.0}\\\n"
		"        amber_prmtop_file=$amber_prmtop ] ] \\\n"
		"\n"
		"\n"
		"write_xyz file= ${ sys_name_id }_opt.xyz coords = ${ sys_name_id }_opt.c\n"
		"read_pdb  file= ${ sys_name_id }.pdb  coords = dummy.coords\n"
		"write_pdb file= ${ sys_name_id }_opt.pdb coords = ${ sys_name_id }_opt.c\n"
		"write_xyz file= ${ sys_name_id }_qm_region_opt.xyz coords = hybrid.${ qm_theory }.coords\n"
		"delete_object hybrid.${ qm_theory }.coords\n"
		"catch {file delete dummy.coords}\n"
		"\n"
		"close $control_input_settings\n";
}

void energy::interfaces::chemshell::sysCallInterface::write_chemshell_file(bool const & sp) const {
	
	//auto qm_atoms = parse_qm_atoms();


	auto o_file = tmp_file_name + ".chm";

	std::ofstream chem_shell_input_stream(o_file);


	chem_shell_input_stream <<
		"global sys_name_id\n"
		"global qm_theory\n"
		"global ftupd\n"
		"\n"
		"set dir .\n"
		"set sys_name_id " << tmp_file_name << "\n"
		"\n"
		"set amber_prmtop " << tmp_file_name << ".prmtop\n"
		"set amber_inpcrd " << tmp_file_name << ".inpcrd\n"
		"\n"
		"set control_input_settings [ open control_input.${sys_name_id}  a ]\n"
		"\n"
		"read_pdb file=${dir}/${sys_name_id}.pdb coords=${dir}/${sys_name_id}.c\n"
		"\n"
		"load_amber_coords inpcrd=$amber_inpcrd prmtop=$amber_prmtop coords=${sys_name_id}.c\n"
		"\n"
		"set embedding_scheme " << Config::get().energy.chemshell.scheme << "\n"
		"\n"
		"set qm_theory " << Config::get().energy.chemshell.qm_theory << "\n"
		"puts $control_input_settings \" QM method: $qm_theory \"\n"
		"\n"
		"set qm_ham " << Config::get().energy.chemshell.qm_ham << "\n"
		"\n"
		"set qm_basis " << Config::get().energy.chemshell.qm_basis << "\n"
		"\n"
		"set qm_ch " << Config::get().energy.chemshell.qm_charge << "\n"
		"\n"
		"set qm_atoms  { " << Config::get().energy.chemshell.qm_atoms << " }\n"
		"\n"
		"set residues [pdb_to_res \"${sys_name_id}.pdb\"]\n"
		"\n"
		"flush $control_input_settings\n"
		"\n";
	if (sp) {
		make_sp_inp(chem_shell_input_stream);
	}
	else {
		make_opt_inp(chem_shell_input_stream);
	}
		

	chem_shell_input_stream.close();


}

void energy::interfaces::chemshell::sysCallInterface::make_opti() const {
	call_chemshell(false);
}

void energy::interfaces::chemshell::sysCallInterface::make_sp()const {
	call_chemshell();
}

std::string energy::interfaces::chemshell::sysCallInterface::find_active_atoms() const {
	
	std::vector<int> indices(coords->size());
	std::iota(indices.begin(), indices.end(), 1);
	std::vector<int> final_vec;

	std::transform(coords->atoms().begin(), coords->atoms().end(), indices.begin(), std::back_inserter(final_vec), 
		[](auto const & a, auto const & b) {
			if (a.fixed()) {
				return 0;
			}
			else {
				return b;
			}
	});

	std::string final_atoms = "";
	for (auto const & i : final_vec) {
		if (i != 0) {
			final_atoms += std::to_string(i) + " ";
		}
	}

	return final_atoms;
}

/*std::vector<std::string> energy::interfaces::chemshell::sysCallInterface::parse_qm_atoms() const {

}*/

void energy::interfaces::chemshell::sysCallInterface::call_chemshell(bool singlepoint) const {
	
	create_pdb();
	write_input(singlepoint);
	actual_call();

}

void energy::interfaces::chemshell::sysCallInterface::actual_call()const {

	std::stringstream chemshell_stream;
	chemshell_stream << Config::get().energy.chemshell.path << " " << tmp_file_name << ".chm";

	auto failcount = 0;

	for (; failcount <= 10; ++failcount) {
		auto ret = scon::system_call(chemshell_stream.str());
		if (ret == 0) {
			break;
		}
		else {
			std::cout << "I am failing to call Chemshell! Are you sure you passed the right chemshell path?\n";
			std::cout << "The path passed is: \"" << Config::get().energy.chemshell.path << "\"\n";
		}
	}

	if (failcount == 10) {
		throw std::runtime_error("10 Chemshell calls failed!");
	}
}

bool energy::interfaces::chemshell::sysCallInterface::check_if_number(std::string const & number) const {

	return !number.empty() && std::find_if(number.cbegin(), number.cend(), [](char n) {
		return n != 'E' && n != 'e' && n != '-' && n != '+' && n != '.' && !std::isdigit(n); //check if the line contains digits, a minus or a dot to determine if its a floating point number
	}) == number.end();

}

coords::float_type energy::interfaces::chemshell::sysCallInterface::read_energy(std::string const & what)const {
	std::ifstream ifile(what + ".energy");

	std::string line;
	while (getline(ifile, line)) {
		std::istringstream iss(line);
		std::vector<std::string> words{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		};
		if (words.size()==0) {
			continue;
		}
		if(check_if_number(words.at(0))){
			return std::stod(words.at(0));
		}
	}

	return 0.0;

}

coords::Representation_3D energy::interfaces::chemshell::sysCallInterface::extract_gradients(std::vector<coords::float_type> const & grads) const {
	coords::Representation_3D new_grads;
	for (auto b = grads.cbegin(); b < grads.cend(); b += 3) {
		new_grads.emplace_back(coords::Cartesian_Point(
			*b,
			*(b+1),
			*(b+2)
		));
	}
	return std::move(new_grads);
}

void energy::interfaces::chemshell::sysCallInterface::read_gradients(std::string const & what) {
	std::ifstream ifile(what + ".gradient");

	std::string line;

	std::vector<coords::float_type> gradients;

	while (getline(ifile, line)) {
		std::istringstream iss(line);
		std::vector<std::string> words{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		};
		if (words.size() == 0) {
			continue;
		}
		if (check_if_number(words.at(0))) {
			gradients.emplace_back(std::stod(words.at(0)));
		}
	}

	auto new_gradients = extract_gradients(gradients);

	coords->swap_g_xyz(new_gradients);

}

bool energy::interfaces::chemshell::sysCallInterface::check_if_line_is_coord(std::vector<std::string> const & coords)const {
	return 
		coords.size() == 4 &&
		check_if_number(coords.at(1)) && 
		check_if_number(coords.at(2)) && 
		check_if_number(coords.at(3));
}

coords::Cartesian_Point energy::interfaces::chemshell::sysCallInterface::make_coords(std::vector<std::string> const & line) const {
	std::vector<std::string> coord_words(line.cbegin() + 1, line.cend());
	coords::Cartesian_Point cp(
		std::stod(coord_words.at(0)),
		std::stod(coord_words.at(1)),
		std::stod(coord_words.at(2))
	);
	return cp;
}

void energy::interfaces::chemshell::sysCallInterface::read_coords(std::string const & what) {
	std::ifstream ifile(what+".coo");

	std::string line;
	coords::Representation_3D xyz;

	while (getline(ifile, line)) {
		std::istringstream iss(line);
		std::vector<std::string> words{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		};
		if(words.size()==0){
			continue;
		}
		
		if (check_if_line_is_coord(words)) {
			xyz.emplace_back(make_coords(words));
		}
	}

	coords->set_xyz(xyz);

	ifile.close();
}

void energy::interfaces::chemshell::sysCallInterface::swap(interface_base & other){}
energy::interface_base * energy::interfaces::chemshell::sysCallInterface::clone(coords::Coordinates * coord_object) const { return new sysCallInterface(*this, coord_object); }
energy::interface_base * energy::interfaces::chemshell::sysCallInterface::move(coords::Coordinates * coord_object) { return new sysCallInterface(*this, coord_object); }

void energy::interfaces::chemshell::sysCallInterface::update(bool const skip_topology){}

coords::float_type energy::interfaces::chemshell::sysCallInterface::e(void) { 
	make_sp();
	return read_energy("energy");
}
coords::float_type energy::interfaces::chemshell::sysCallInterface::g(void) {
	make_sp();
	read_gradients("energy");
	return read_energy("energy");
}
coords::float_type energy::interfaces::chemshell::sysCallInterface::h(void) {
	return 0.0; 
}
coords::float_type energy::interfaces::chemshell::sysCallInterface::o(void) {
	make_opti();
	read_gradients("dl-find");
	read_coords("dl-find");
	return read_energy("dl-find"); 
}

void energy::interfaces::chemshell::sysCallInterface::print_E(std::ostream&) const{}

void energy::interfaces::chemshell::sysCallInterface::print_E_head(std::ostream&, bool const endline) const {}

void energy::interfaces::chemshell::sysCallInterface::print_E_short(std::ostream&, bool const endline) const {}

void energy::interfaces::chemshell::sysCallInterface::print_G_tinkerlike(std::ostream & S, bool const aggregate) const {

	coords::Representation_3D gradients;

	coords->get_g_xyz(gradients);

	for (auto const & grad : gradients) {
		S << grad << "\n";
	}

}

void energy::interfaces::chemshell::sysCallInterface::to_stream(std::ostream&) const {}