#include "model/model.hpp"
#include "model/model_parsers.hpp"
#include "constraints/steady_space.hpp"
#include "constraints/space_solver.hpp"
#include "general/program_options.hpp"

/* Turn the model file into a model object. */
Model parseModel(const string & path_to_model) {
	Model result;
	// Parse the file path
	bfs::path model_path(path_to_model);
	ModelParsers::testPath(model_path);
	result.name = model_path.stem().string();
	// Read the file and control its syntax
	vector<string> model_content = ModelParsers::readModel(model_path);
	rng::sort(model_content);
	ModelParsers::control_syntax(model_content);
	// Translate the into a model and control semantics of the rules
	result.species.resize(model_content.size());
	rng::transform(model_content, result.species.begin(), ModelParsers::obtainSpecie);
	ModelParsers::control_semantics(result.species);
	// Get the global maximum
	result.max_value = rng::max_element(result.species, [](Specie & a, Specie & b) {return a.max_val < b.max_val; })->max_val;

	return result;
}

/* Create a steady state solver, solve the constraints. */
void solveSteadyStates(const string & path_to_model, const Model & model) {
	// Create the output file and label columns
	bfs::path model_path(path_to_model);
	string output_file_name = model_path.parent_path().string() + model.name + "_stable.csv";
	ofstream output_file(output_file_name, ios::out);
	rng::for_each(model.species, [&output_file](Specie spec){output_file << spec.name << ","; });
	output_file << endl;
	// Create the solver together with the constraints
	SpaceSolver<SteadySpace> solver(new SteadySpace(model.species.size(), model.max_value));
	for (const size_t i : cscope(model.species))
		solver->boundSpecie(i, model.species[i].max_val);
	solver->applyModel(model);
	// Output the results
	vector<int> result = solver.next();
	while (!result.empty()) {
		rng::for_each(result, [&output_file](int i){output_file << i << ","; });
		output_file << endl;
		result = solver.next();
	}
}

int main(int argc, char ** argv) {
	bpo::variables_map program_options;
	string path_to_model;
	try {
		program_options = parseProgramOptions(argc, argv);
		path_to_model = program_options["model"].as<string>();
	}
	catch (exception & e) {
		BOOST_LOG_TRIVIAL(error) << "An exception occurred while parsing input options: " << e.what();
		exit(1);
	}
	
	Model model;
	try {
		model = parseModel(path_to_model);
	}
	catch (exception & e) {
		BOOST_LOG_TRIVIAL(error) << "An exception occurred while reading the model file \"" << path_to_model << "\":" << e.what();
		exit(2);
	}

	try {
		if (program_options.count("steady"))
			solveSteadyStates(path_to_model, model);
	}
	catch (exception & e) {
		BOOST_LOG_TRIVIAL(error) << "An exception occurred while computing the steady states: " << e.what();
		exit(3);
	}


	return 0;
}
